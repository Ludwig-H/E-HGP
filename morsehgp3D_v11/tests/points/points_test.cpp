// Portes unitaires du module points (tranche S9) et de l'index d'ancetres de tower :
//   ancestors    tower::AncestorIndex contre la remontee naive (profondeur, ancetre a une profondeur, plus petit
//                ancetre commun de toutes les paires, plus haut ancetre de rang <= r) sur les forets d'ordre 1 a 4 de
//                nuages tires ;
//   determinism  points::hang sans Pool et sur des Pools de 1, 2 et 4 fils : colonnes identiques ;
//   refusals     K = n a K >= 2 (parameter_out_of_range), m hors de 1..K+1 (parameter_out_of_range), budget trop
//                petit (memory_budget, budget rendu) ; la raison points_invariant est un invariant viole (code 3) ;
//   budget       un appel conforme rend son budget de travail : seul le produit reste reserve, puis rien.
#include <memory>
#include <random>
#include <vector>

#include "../supports/supports_support.hpp"
#include "points/internal.hpp"
#include "points/points.hpp"
#include "sched/sched.hpp"
#include "test.hpp"

using namespace mhgp11;
using supports_test::Points;

namespace {

Points drawn(u32 count, u32 side, u32 seed) {
  std::mt19937 rng(seed);
  Points out;
  while (out.size() < count) {
    const supports_test::Xyz p{static_cast<u32>(rng() % side), static_cast<u32>(rng() % side),
                               static_cast<u32>(rng() % side)};
    if (std::find(out.begin(), out.end(), p) == out.end()) out.push_back(p);
  }
  return out;
}

Result<OrderTree> tree_of(const Points& points, Order k, MemoryBudget& budget) {
  auto domain = supports_test::domain_of(points, k, budget);
  if (!domain.ok()) return domain.outcome();
  return build_order(std::move(domain.value()), k, budget);
}

// Colonnes d'une pendaison, pour comparer deux appels.
std::vector<u32> columns(const points::PointHierarchy& h) {
  std::vector<u32> out;
  for (const auto values : {h.t(), h.m(), h.q(), h.owner(), h.floor(), h.levels(), h.tree().plateau_t(),
                            h.tree().plateau_m(), h.tree().plateau_q(), h.tree().block_plateau(),
                            h.tree().block_parent(), h.tree().site_block(), h.tree().site_plateau()}) {
    out.push_back(static_cast<u32>(values.size()));
    out.insert(out.end(), values.begin(), values.end());
  }
  for (const u8 flag : h.strict()) out.push_back(flag);
  return out;
}

}  // namespace

MHGP11_TEST(ancestors, 2000) {
  for (u32 seed = 1; seed <= 6; ++seed) {
    for (Order k = 1; k <= 4; ++k) {
      MemoryBudget budget(MemoryBudget::kUnlimited);
      auto tree = tree_of(drawn(14 + 3 * seed, 9, seed), k, budget);
      REQUIRE(tree.ok());
      const auto nodes = tree.value().forest().nodes();
      auto index = AncestorIndex::build(tree.value().forest(), budget);
      REQUIRE(index.ok());
      const auto chain = [&](u32 v) {
        std::vector<u32> up{v};
        while (nodes[up.back()].parent != NodeIdx{kNone}) up.push_back(idx(nodes[up.back()].parent));
        return up;
      };
      for (u32 a = 0; a < nodes.size(); ++a) {
        const auto ca = chain(a);
        CHECK_EQ(index.value().depth(NodeIdx{a}), ca.size() - 1);
        for (u32 d = 0; d < ca.size(); ++d) CHECK_EQ(idx(index.value().at_depth(NodeIdx{a}, d)), ca[ca.size() - 1 - d]);
        for (u32 b = 0; b < nodes.size(); b += 3) {
          const auto cb = chain(b);
          u32 naive = kNone;
          for (const u32 x : ca)
            if (naive == kNone && std::find(cb.begin(), cb.end(), x) != cb.end()) naive = x;
          CHECK_EQ(idx(index.value().lca(NodeIdx{a}, NodeIdx{b})), naive);
        }
        const u32 limit = idx(nodes[ca.back()].rank) / 2;
        NodeIdx top{a};
        const Outcome got = index.value().highest(NodeIdx{a}, [&](NodeIdx x, bool& ok) -> Outcome {
          ok = idx(nodes[idx(x)].rank) <= limit;
          return {};
        }, top);
        u32 expect = a;
        for (std::size_t j = 1; j < ca.size() && idx(nodes[ca[j]].rank) <= limit; ++j) expect = ca[j];
        CHECK(got.ok());
        CHECK_EQ(idx(top), expect);
      }
    }
  }
}

MHGP11_TEST(determinism, 60) {
  for (u32 seed = 1; seed <= 4; ++seed) {
    for (Order k = 1; k <= 4; ++k) {
      MemoryBudget budget(MemoryBudget::kUnlimited);
      auto tree = tree_of(drawn(40 + 10 * seed, 12, 100 + seed), k, budget);
      REQUIRE(tree.ok());
      auto serial = points::hang(tree.value(), budget);
      REQUIRE(serial.ok());
      const auto want = columns(serial.value());
      for (const u32 w : {1u, 2u, 4u}) {
        auto pool = sched::make_pool({w});
        REQUIRE(pool.ok());
        auto got = points::hang(tree.value(), budget, pool.value().get());
        REQUIRE(got.ok());
        CHECK(columns(got.value()) == want);
      }
    }
  }
}

// Tri d'un groupe strict (audit d448b3d03) : refus injecte apres plusieurs comparaisons reussies, sur 17 a 64
// entrees ; le refus est rendu tel quel, le tableau reste une permutation (aucune lecture hors bornes), et sans refus
// le resultat est l'ordre total attendu.
MHGP11_TEST(sort_refusal, 528) {
  for (u32 n = 17; n <= 64; ++n) {
    for (const u32 fail_at : {4u, 9u, 30u}) {
      std::vector<u32> keys(n), order(n);
      for (u32 i = 0; i < n; ++i) keys[i] = (i * 7919u) % 13u, order[i] = i;
      u32 calls = 0;
      bool inbounds = true;
      const Outcome got = points_detail::heap_sort_until_refusal(order.data(), n, [&](u32 a, u32 b, bool& less) {
        inbounds = inbounds && a < n && b < n;
        if (++calls == fail_at) return Outcome{fail(Reason::radical_sign_budget)};
        less = keys[a] < keys[b] || (keys[a] == keys[b] && a < b);
        return Outcome{};
      });
      CHECK(!got.ok() && got.reason == Reason::radical_sign_budget);
      CHECK(inbounds);
      std::vector<u32> sorted = order;
      std::sort(sorted.begin(), sorted.end());
      bool permutation = true;
      for (u32 i = 0; i < n; ++i) permutation = permutation && sorted[i] == i;
      CHECK(permutation);
    }
    std::vector<u32> keys(n), order(n);
    for (u32 i = 0; i < n; ++i) keys[i] = (i * 7919u) % 13u, order[i] = i;
    const Outcome ok = points_detail::heap_sort_until_refusal(order.data(), n, [&](u32 a, u32 b, bool& less) {
      less = keys[a] < keys[b] || (keys[a] == keys[b] && a < b);
      return Outcome{};
    });
    CHECK(ok.ok());
    bool ordered = true;
    for (u32 i = 1; i < n; ++i)
      ordered = ordered && (keys[order[i - 1]] < keys[order[i]] || (keys[order[i - 1]] == keys[order[i]] &&
                                                                    order[i - 1] < order[i]));
    CHECK(ordered);
  }
}

MHGP11_TEST(refusals, 12) {
  const Points five = {{6, 2, 0}, {0, 0, 0}, {0, 4, 0}, {12, 0, 0}, {12, 4, 0}};
  {
    MemoryBudget budget(MemoryBudget::kUnlimited);
    auto tree = tree_of(five, 5, budget);  // K = n = 5
    REQUIRE(tree.ok());
    auto got = points::hang(tree.value(), budget);
    CHECK(!got.ok() && got.outcome().reason == Reason::parameter_out_of_range);
  }
  {
    MemoryBudget budget(MemoryBudget::kUnlimited);
    auto tree = tree_of(five, 2, budget);
    REQUIRE(tree.ok());
    for (const u32 m : {0u, 4u, 9u}) {
      auto got = points::hang_qualified(tree.value(), m, budget);
      CHECK(!got.ok() && got.outcome().reason == Reason::parameter_out_of_range);
    }
    CHECK(points::hang_qualified(tree.value(), 1, budget).ok());
    CHECK(points::hang_qualified(tree.value(), 3, budget).ok());
  }
  {
    // Budget trop petit pour la pendaison : memory_budget, rien ne reste reserve au-dela de l'arbre.
    MemoryBudget tree_budget(MemoryBudget::kUnlimited);
    auto tree = tree_of(drawn(60, 12, 7), 3, tree_budget);
    REQUIRE(tree.ok());
    for (const u64 limit : {u64{64}, u64{4096}, u64{65536}}) {
      MemoryBudget budget(limit);
      auto got = points::hang(tree.value(), budget);
      CHECK(!got.ok() && got.outcome().reason == Reason::memory_budget);
      CHECK_EQ(budget.used(), 0u);
    }
  }
  CHECK(status_of(Reason::points_invariant) == Status::invariant_violated);
  CHECK_EQ(exit_code(fail(Reason::points_invariant)), 3);
}

MHGP11_TEST(budget, 8) {
  MemoryBudget tree_budget(MemoryBudget::kUnlimited);
  auto tree = tree_of(drawn(80, 14, 11), 4, tree_budget);
  REQUIRE(tree.ok());
  MemoryBudget budget(MemoryBudget::kUnlimited);
  {
    auto got = points::hang(tree.value(), budget);
    REQUIRE(got.ok());
    const points::PointHierarchy& h = got.value();
    const u64 n = h.t().size();
    const u64 product = 4 * (5 * n + h.levels().size() + 3 * h.tree().plateau_t().size() +
                             2 * h.tree().block_plateau().size() + 2 * n) + n;
    CHECK_EQ(budget.used(), product);
    CHECK(h.tree().block_plateau().size() <= 2 * n - 1);
    CHECK(h.qualification() == 5 && h.order() == 4);
    CHECK(h.stats().incidences > 0 && h.stats().roots > 0);
  }
  CHECK_EQ(budget.used(), 0u);
  CHECK(budget.released().ok());
  CHECK(budget.peak() > 0);
}

MHGP11_TEST_MAIN()
