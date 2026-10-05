// Arbre d'ordre K seul (build_order, tranche S3) : identite I10 avec build_full(...).order(K) sur le meme domaine
// (noeuds, enfants, rangs, cles de naissance, champs logiques du registre), K = 1..5, W1/W2/W4, voie serielle et
// lots ; rattachement identique quelles que soient la voie et le nombre de fils ; refus avant tout effet.
#include "order_tree_support.hpp"
#include "test.hpp"
using namespace order_tree_test;

namespace {
Result<OrderTree> order_of(const Input& input, Order kmax, Order k, MemoryBudget& owner, MemoryBudget& work,
                           const FullParams& params = {}, sched::Pool* pool = nullptr) {
  auto domain = domain_of(input, owner, kmax);
  if (!domain.ok()) return domain.outcome();
  return build_order(std::move(domain.value()), k, work, params, pool);
}
}  // namespace

MHGP11_TEST(identity, 5000) {
  auto p1 = sched::make_pool({1}), p2 = sched::make_pool({2}), p4 = sched::make_pool({4});
  REQUIRE(p1.ok() && p2.ok() && p4.ok());
  u64 compared = 0, narrowed = 0, attachments = 0, balls = 0;
  for (const auto& cloud : clouds()) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    const Input input(cloud.points);
    // Reference : build_full serielle sur le meme domaine (Cat_kmax), et sur Cat_k pour l'arbre propre a l'ordre.
    auto full = tower_of(input, cloud.kmax, owner, work); REQUIRE(full.ok());
    const u64 held = work.used();
    for (Order k = 1; k <= cloud.kmax; ++k) {
      std::optional<OrderTree> first;
      auto own = tower_of(input, k, owner, work); REQUIRE(own.ok());
      // Chaque variante de lots tourne sur W1, W2 ou W4 selon (variante + K) : les trois Pool voient toutes les
      // variantes au fil des ordres.
      const std::array<sched::Pool*, 3> pools{p1.value().get(), p2.value().get(), p4.value().get()};
      const auto all = variants();
      // Nuages de plus de 14 points : une variante sur trois (cout sous ASan) ; les fixtures les voient toutes.
      const u64 step = cloud.points.size() > 14 ? 3 : 1;
      for (u64 v = 0; v < all.size(); v += step) {
        const auto& params = all[v];
        sched::Pool* pool = v == 0 ? nullptr : pools[(v + k) % 3];
        auto order = order_of(input, cloud.kmax, k, owner, work, params, pool);
        REQUIRE(order.ok());
        const OrderForest& forest = order.value().forest();
        CHECK(same_tree(forest, full.value().order(k)));
        CHECK(logical(forest.ledger()) == logical(full.value().order(k).ledger()));
        CHECK(forest.lower().empty()); CHECK(structure(forest));
        CHECK_EQ(order.value().order(), k);
        ++compared;
        if (v == 0 || v + step >= all.size()) {
          // Meme arbre sur Cat_k (domaine propre a l'ordre, cles de naissance propres a Cat_k).
          auto narrow = order_of(input, k, k, owner, work, params, pool); REQUIRE(narrow.ok());
          CHECK(same_tree(narrow.value().forest(), own.value().order(k)));
          CHECK(logical(narrow.value().forest().ledger()) == logical(own.value().order(k).ledger()));
          ++narrowed;
        }
        if (!first) { balls += order.value().attachment().size(); first.emplace(std::move(order.value())); continue; }
        // Rattachement identique quels que soient la voie, les memos, la table et le nombre de fils.
        CHECK(same_attachment(order.value().attachment(), first->attachment()));
        ++attachments;
      }
      CHECK(first.has_value());
    }
    CHECK_EQ(work.used(), held);
  }
  CHECK(compared > 500); CHECK(narrowed > 100); CHECK(balls > 10000);
  std::printf("order_identity compared=%llu narrowed=%llu attachments=%llu balls=%llu\n",
              (unsigned long long)compared, (unsigned long long)narrowed, (unsigned long long)attachments,
              (unsigned long long)balls);
}

// Identite avec build_full aux memes parametres non concurrents (verticales en moins) : W4, lots et voie serielle.
MHGP11_TEST(same_params, 1700) {
  auto pool = sched::make_pool({4}); REQUIRE(pool.ok());
  u64 compared = 0;
  for (const auto& cloud : clouds()) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    const Input input(cloud.points);
    const auto all = variants();
    // Une variante sur deux (une sur quatre au-dela de 14 points) : serielle, puis lots alternes.
    const u64 step = cloud.points.size() > 14 ? 4 : 2;
    for (u64 v = 0; v < all.size(); v += step) {
      const auto& params = all[v];
      sched::Pool* p = params.regular_batch_capacity == 0 ? nullptr : pool.value().get();
      auto domain = domain_of(input, owner, cloud.kmax); REQUIRE(domain.ok());
      auto full = build_full(std::move(domain.value()), work, nullptr, params, p); REQUIRE(full.ok());
      for (Order k = 1; k <= cloud.kmax; ++k) {
        auto other = domain_of(input, owner, cloud.kmax); REQUIRE(other.ok());
        auto order = build_order(std::move(other.value()), k, work, params, p); REQUIRE(order.ok());
        CHECK(same_tree(order.value().forest(), full.value().order(k)));
        CHECK(logical(order.value().forest().ledger()) == logical(full.value().order(k).ledger()));
        ++compared;
      }
    }
  }
  CHECK(compared > 350);
  std::printf("order_same_params compared=%llu\n", (unsigned long long)compared);
}

MHGP11_TEST(refusals, 100) {
  MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
  const Input input(clouds()[1].points);
  auto pool = sched::make_pool({4}); REQUIRE(pool.ok());
  auto domain = domain_of(input, owner, 3); REQUIRE(domain.ok());
  const auto* xyz = domain.value().index().cloud().x().data();
  OrderTimings timings{7, 11, 13, 17}; const auto saved = timings;
  u64 attach = 99;
  FullParams batch; batch.regular_batch_capacity = 8; batch.descent_lanes = 2;
  auto refuse = [&](FullDomain& d, Order k, FullParams params, sched::Pool* p, Reason reason, MemoryBudget& b) {
    const auto* sites = d.index().cloud().x().data();
    const u64 before = b.used();
    auto refused = build_order(std::move(d), k, b, params, p, &timings, &attach);
    CHECK(!refused.ok()); CHECK_EQ(refused.outcome().reason, reason);
    CHECK(d.index().cloud().x().data() == sites);  // domaine conserve, non deplace
    CHECK(timings == saved); CHECK_EQ(attach, 99u); CHECK_EQ(b.used(), before);
  };
  refuse(domain.value(), 0, {}, nullptr, Reason::parameter_out_of_range, work);
  refuse(domain.value(), 4, {}, nullptr, Reason::parameter_out_of_range, work);  // k > kmax
  FullParams concurrent = batch; concurrent.concurrent_orders = true;
  refuse(domain.value(), 2, concurrent, pool.value().get(), Reason::parameter_out_of_range, work);
  FullParams verticals = batch; verticals.parallel_verticals = true;
  refuse(domain.value(), 2, verticals, pool.value().get(), Reason::parameter_out_of_range, work);
  FullParams reuse; reuse.reuse_regular_verticals = true;
  refuse(domain.value(), 2, reuse, nullptr, Reason::parameter_out_of_range, work);
  refuse(domain.value(), 2, batch, nullptr, Reason::parameter_out_of_range, work);  // lots sans Pool
  FullParams lanes = batch; lanes.descent_lanes = 0;
  refuse(domain.value(), 2, lanes, pool.value().get(), Reason::parameter_out_of_range, work);
  {
    // K > n : deux sites, Cat_3 ; k = 3 refuse, k = 2 accepte (une seule naissance, aucune cellule).
    auto small = domain_of(Input({{0,0,0},{2,0,0}}), owner, 3); REQUIRE(small.ok());
    refuse(small.value(), 3, {}, nullptr, Reason::parameter_out_of_range, work);
    auto two = build_order(std::move(small.value()), 2, work); REQUIRE(two.ok());
    CHECK_EQ(two.value().forest().births(), 1u); CHECK_EQ(two.value().attachment().size(), 1u);
    CHECK(two.value().attachment().role()[0] == BallRole::birth);
    CHECK_EQ(two.value().attachment().prior_offsets().size(), 2u);
  }
  CHECK_EQ(work.used(), 0u);
  // Budget trop court : refus memory_budget, rien de retenu, diagnostics intacts ; domaine neuf a chaque essai. Voie
  // serielle : pic exact deterministe, refus a pic - 1 et succes au pic ; lots : refus aux petits plafonds.
  u64 exact = 0;
  {
    MemoryBudget probe(MemoryBudget::kUnlimited);
    auto fresh = domain_of(input, owner, 3); REQUIRE(fresh.ok());
    auto made = build_order(std::move(fresh.value()), 2, probe); REQUIRE(made.ok());
    exact = probe.peak();
  }
  std::printf("order_refusals pic_exact=%llu\n", static_cast<unsigned long long>(exact));
  REQUIRE(exact > 256);
  u64 refusals = 0, successes = 0;
  struct Trial { u64 limit; bool lots; };
  for (const Trial trial : {Trial{64, true}, Trial{512, true}, Trial{64, false}, Trial{exact / 2, false},
                            Trial{exact - 1, false}, Trial{exact, false}}) {
    auto fresh = domain_of(input, owner, 3); REQUIRE(fresh.ok());
    const auto* sites = fresh.value().index().cloud().x().data();
    MemoryBudget tight(trial.limit);
    timings = saved; attach = 99;
    {
      auto made = build_order(std::move(fresh.value()), 2, tight, trial.lots ? batch : FullParams{},
                              trial.lots ? pool.value().get() : nullptr, &timings, &attach);
      if (made.ok()) {
        ++successes; CHECK_EQ(trial.limit, exact);
      } else {
        CHECK_EQ(made.outcome().reason, Reason::memory_budget); ++refusals;
        CHECK(timings == saved); CHECK_EQ(attach, 99u);
        CHECK(fresh.value().index().cloud().x().data() == sites);
      }
    }
    CHECK_EQ(tight.used(), 0u);
  }
  CHECK_EQ(refusals, 5u); CHECK_EQ(successes, 1u);
  timings = saved; attach = 99;
  // Succes : diagnostics publies, domaine transfere, budget rendu a la destruction du resultat.
  {
    auto made = build_order(std::move(domain.value()), 2, work, batch, pool.value().get(), &timings, &attach);
    REQUIRE(made.ok());
    CHECK(attach != 99u); CHECK(!(timings == saved));
    CHECK_EQ(timings.verticals_ns, 0u); CHECK(timings.regular_batches >= 1u);
    CHECK(made.value().domain().index().cloud().x().data() == xyz);
    CHECK(work.used() > 0u);
  }
  CHECK_EQ(work.used(), 0u);
}

MHGP11_TEST_MAIN()
