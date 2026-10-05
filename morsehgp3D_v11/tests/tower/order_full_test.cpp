// Arbre d'ordre K tire de FULL (build_order_full, livraison L2b de docs/SORTIES.md, paragraphe 11) : journal des
// graines pose sur le constructeur de l'ordre K de build_full, voie non concurrente (pilote : serielle, lots, avec ou
// sans verticales paralleles et reemploi) et voie concurrente (par etages a W3, pipeline a W12), puis extraction de la
// seule foret d'ordre K. Identite avec build_order (voie serielle de reference) : foret (I10), champs logiques du
// registre, rattachement (WindowAttachment entier) et registres du journal (cellules, graines, empreinte), sur les
// nuages bornes de order_tree_support.hpp a tous leurs ordres ; aucune verticale gardee, budget rendu. Refus avant
// tout effet, budgets trop courts sans reservation restante ni diagnostic ecrit.
#include "order_tree_support.hpp"
#include "test.hpp"
using namespace order_tree_test;

namespace {

struct Route {
  const char* name;
  FullParams params;
  u32 workers;  // 0 : sans Pool
};

// Voies de FULL exercees. Le masque 16379 de la facade (api_detail::full_params) est la derniere, sur W3 et W12.
std::vector<Route> routes() {
  std::vector<Route> out;
  out.push_back({"serielle", FullParams{}, 0});
  FullParams lots;
  lots.regular_batch_capacity = 4096; lots.descent_lanes = 4; lots.dense_birth_lookup = true;
  out.push_back({"lots", lots, 3});
  FullParams verticales = lots;
  verticales.parallel_verticals = true; verticales.reuse_regular_verticals = true; verticales.population_lookup = true;
  verticales.reuse_census_workspace = true;
  out.push_back({"lots_verticales", verticales, 3});
  FullParams petits = lots;
  petits.regular_batch_capacity = 1; petits.memo_capacity = 8; petits.lane_memo_capacity = 8;
  out.push_back({"lots_memo", petits, 2});
  FullParams concurrent = lots;
  concurrent.concurrent_orders = true;
  out.push_back({"etages", concurrent, 3});
  FullParams facade;
  facade.regular_batch_capacity = 4096; facade.descent_lanes = 48; facade.parallel_verticals = true;
  facade.reuse_census_workspace = true; facade.dense_birth_lookup = true; facade.reuse_regular_verticals = true;
  facade.population_lookup = true; facade.concurrent_orders = true;
  out.push_back({"masque16379_w3", facade, 3});
  out.push_back({"masque16379_w12", facade, 12});
  return out;
}

}  // namespace

MHGP11_TEST(identity, 11500) {
  std::array<std::unique_ptr<sched::Pool>, 13> pools;
  for (u32 w : {2u, 3u, 12u}) {
    auto made = sched::make_pool({w}); REQUIRE(made.ok());
    pools[w] = std::move(made.value());
  }
  u64 compared = 0, pipelined = 0, staged = 0, sequential = 0, cells = 0, seeds = 0, balls = 0;
  for (const auto& cloud : clouds()) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    const Input input(cloud.points);
    for (Order k = 1; k <= cloud.kmax; ++k) {
      // Reference : arbre d'ordre K seul, voie serielle, sur Cat_kmax ; puis sur Cat_k (domaine de la facade).
      for (const Order kmax : {cloud.kmax, k}) {
        auto domain = domain_of(input, owner, kmax); REQUIRE(domain.ok());
        SeedLogRegisters want;
        auto reference = build_order(std::move(domain.value()), k, work, {}, nullptr, nullptr, nullptr, &want);
        REQUIRE(reference.ok());
        const u64 held = work.used();
        for (const auto& route : routes()) {
          sched::Pool* pool = route.workers == 0 ? nullptr : pools[route.workers].get();
          auto fresh = domain_of(input, owner, kmax); REQUIRE(fresh.ok());
          FullTimings timings;
          SeedLogRegisters got;
          u64 attach = 0;
          const u64 before = work.used();
          {
            auto tree = build_order_full(std::move(fresh.value()), k, work, route.params, pool, &timings, &attach,
                                         &got);
            REQUIRE(tree.ok());
            const OrderForest& forest = tree.value().forest();
            CHECK_EQ(tree.value().order(), k);
            CHECK(same_tree(forest, reference.value().forest()));
            CHECK(logical(forest.ledger()) == logical(reference.value().forest().ledger()));
            CHECK(same_attachment(tree.value().attachment(), reference.value().attachment()));
            CHECK(got == want);
            CHECK(forest.lower().empty());  // verticales rendues
            CHECK(structure(forest));
            CHECK_EQ(tree.value().domain().catalogue().kmax(), kmax);
            // Voie effectivement jouee (diagnostic de build_full) : pipeline seulement a W >= 2 kmax, kmax >= 2.
            if (route.params.concurrent_orders) {
              CHECK(timings.concurrent_orders);
              if (timings.pipeline_lanes != 0) ++pipelined; else ++staged;
              if (route.workers >= 2 * kmax && kmax >= 2) CHECK(timings.pipeline_lanes != 0);
            } else {
              CHECK(!timings.concurrent_orders); CHECK_EQ(timings.pipeline_lanes, 0u);
              ++sequential;
            }
          }
          CHECK_EQ(work.used(), before);  // foret, rattachement et domaine du resultat rendus a sa destruction
          ++compared;
        }
        cells += want.cells; seeds += want.seeds; balls += reference.value().attachment().size();
        CHECK_EQ(work.used(), held);
      }
    }
  }
  CHECK(compared >= 882); CHECK(pipelined >= 112); CHECK(staged >= 266); CHECK(sequential >= 504);
  CHECK(cells >= 28680); CHECK(seeds >= 89014); CHECK(balls >= 45896);
  std::printf("order_full_identity compared=%llu pipeline=%llu etages=%llu non_concurrent=%llu cells=%llu "
              "seeds=%llu balls=%llu\n", (unsigned long long)compared, (unsigned long long)pipelined,
              (unsigned long long)staged, (unsigned long long)sequential, (unsigned long long)cells,
              (unsigned long long)seeds, (unsigned long long)balls);
}

// Refus : parametres (k nul, k > kmax, lots sans Pool, voies sans Pool ou sans lots) avant tout effet ; budgets trop
// courts (voie serielle : pic exact deterministe, refus a pic - 1 et succes au pic ; voie concurrente : refus aux
// petits plafonds), sans reservation restante, domaine conserve, diagnostics intacts.
MHGP11_TEST(refusals, 100) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  const Input input(clouds()[3].points);
  auto pool = sched::make_pool({3}); REQUIRE(pool.ok());
  FullTimings timings; timings.memo_capacity = 77; const FullTimings saved = timings;
  u64 attach = 99;
  SeedLogRegisters registers{5, 6, 7}; const SeedLogRegisters kept = registers;
  auto refuse = [&](Order kmax, Order k, FullParams params, sched::Pool* p, Reason reason, MemoryBudget& b) {
    auto domain = domain_of(input, owner, kmax); REQUIRE(domain.ok());
    const auto* sites = domain.value().index().cloud().x().data();
    const u64 before = b.used();
    auto refused = build_order_full(std::move(domain.value()), k, b, params, p, &timings, &attach, &registers);
    CHECK(!refused.ok()); CHECK_EQ(refused.outcome().reason, reason);
    CHECK(domain.value().index().cloud().x().data() == sites);  // domaine conserve, non deplace
    CHECK(timings == saved); CHECK_EQ(attach, 99u); CHECK(registers == kept); CHECK_EQ(b.used(), before);
  };
  FullParams lots; lots.regular_batch_capacity = 8; lots.descent_lanes = 2;
  refuse(4, 0, {}, nullptr, Reason::parameter_out_of_range, work);
  refuse(4, 5, {}, nullptr, Reason::parameter_out_of_range, work);  // k > kmax
  refuse(4, 2, lots, nullptr, Reason::parameter_out_of_range, work);  // lots sans Pool
  FullParams lanes = lots; lanes.descent_lanes = 0;
  refuse(4, 2, lanes, pool.value().get(), Reason::parameter_out_of_range, work);
  FullParams bare; bare.concurrent_orders = true;  // ordres concurrents sans lots
  refuse(4, 2, bare, pool.value().get(), Reason::parameter_out_of_range, work);
  {
    // kmax > n : deux sites, Cat_3 refuse par build_full avant l'ordre demande.
    auto small = domain_of(Input({{0,0,0},{2,0,0}}), owner, 3); REQUIRE(small.ok());
    auto refused = build_order_full(std::move(small.value()), 2, work);
    CHECK(!refused.ok()); CHECK_EQ(refused.outcome().reason, Reason::parameter_out_of_range);
  }
  CHECK_EQ(work.used(), 0u);
  u64 exact = 0;
  {
    MemoryBudget probe(MemoryBudget::kUnlimited);
    auto fresh = domain_of(input, owner, 4); REQUIRE(fresh.ok());
    auto made = build_order_full(std::move(fresh.value()), 3, probe); REQUIRE(made.ok());
    exact = probe.peak();
  }
  std::printf("order_full_refusals pic_exact=%llu\n", static_cast<unsigned long long>(exact));
  REQUIRE(exact > 1024);
  FullParams concurrent = lots; concurrent.concurrent_orders = true; concurrent.regular_batch_capacity = 4096;
  u64 refusals = 0, successes = 0;
  struct Trial { u64 limit; bool concurrent; };
  for (const Trial trial : {Trial{64, true}, Trial{exact / 4, true}, Trial{exact / 2, true}, Trial{64, false},
                            Trial{exact / 3, false}, Trial{exact / 2, false}, Trial{exact - 1, false},
                            Trial{exact, false}}) {
    auto fresh = domain_of(input, owner, 4); REQUIRE(fresh.ok());
    const auto* sites = fresh.value().index().cloud().x().data();
    MemoryBudget tight(trial.limit);
    timings = saved; attach = 99; registers = kept;
    {
      auto made = build_order_full(std::move(fresh.value()), 3, tight, trial.concurrent ? concurrent : FullParams{},
                                   trial.concurrent ? pool.value().get() : nullptr, &timings, &attach, &registers);
      if (made.ok()) {
        ++successes; CHECK_EQ(trial.limit, exact); CHECK(!(registers == kept));
      } else {
        CHECK_EQ(made.outcome().reason, Reason::memory_budget); ++refusals;
        CHECK(timings == saved); CHECK_EQ(attach, 99u); CHECK(registers == kept);
        CHECK(fresh.value().index().cloud().x().data() == sites);
      }
    }
    CHECK_EQ(tight.used(), 0u);
  }
  CHECK_EQ(refusals, 7u); CHECK_EQ(successes, 1u);
}

MHGP11_TEST_MAIN()
