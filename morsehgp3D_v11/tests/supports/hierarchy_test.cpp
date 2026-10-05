// Portes unitaires de l'assemblage de la hierarchie des supports (tranche S6b) :
//   fixtures     petits nuages (fixtures de la specification, temoins D2 et E5 de l'auditeur, cube, octaedre, grilles
//                et nuages tires) a tous leurs ordres : juge complet (I5, I6, I11, rattachement), couverture gravee ;
//   determinism  meme empreinte sans Pool et sur des Pools de 1, 2 et 4 fils, arbre par la voie serielle ou par lots ;
//   permutation  entree permutee et PointId reetiquetes (non denses, 2^32 - 1 compris) : meme empreinte ;
//   sphere5      admission a 24 sites (x^2 + y^2 + z^2 = 5 translatee de (2, 2, 2)) : la boule centrale publie ses 828
//                supports (12, 24, 792) et les comptes des auditeurs de K1 a K3 ;
//   sphere9      refus de l'APPEL ENTIER a 25 sites (25 des 30 sites de x^2 + y^2 + z^2 = 9, les six axiaux gardes) :
//                support_shell_capacity, rien d'alloue (pic nul), diagnostics intacts ;
//   admission    formule d'admission recalculee ici (hierarchy_support.hpp) : admis = formule, pic = premier + second
//                etage exactement ; sous le premier etage d'un octet, refus memory_budget SANS AUCUNE allocation (le
//                pic reste nul : une formule qui oublierait le brouillon ou la liste d'un fil allouerait d'abord les
//                sorties) ; sous le total d'un octet, refus au second etage, budget rendu ; au total, succes. Pools
//                de 1, 2 et 4 fils (le terme par fil est multiplie par le nombre de fils).
#include <memory>
#include <optional>
#include <vector>

#include "hierarchy_support.hpp"
#include "sched/sched.hpp"
#include "supports_support.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace hierarchy_test;
using supports_test::Points;
using supports_test::Xyz;

namespace {

Points grid(u32 side, u32 step, u32 count) {
  Points out;
  for (u32 x = 0; x < side && out.size() < count; ++x)
    for (u32 y = 0; y < side && out.size() < count; ++y)
      for (u32 z = 0; z < 2 && out.size() < count; ++z) out.push_back({x * step, y * step, z * step});
  return out;
}

Points drawn(u32 count, u32 side, u32 seed) {
  Points out;
  u64 state = seed;
  while (out.size() < count) {
    state = state * 6364136223846793005ull + 1442695040888963407ull;
    const Xyz p{static_cast<u32>((state >> 20) % side), static_cast<u32>((state >> 36) % side),
                static_cast<u32>((state >> 50) % side)};
    if (std::find(out.begin(), out.end(), p) == out.end()) out.push_back(p);
  }
  return out;
}

Points sphere(i32 r2, u32 t) {
  Points out;
  for (i32 x = -3; x <= 3; ++x)
    for (i32 y = -3; y <= 3; ++y)
      for (i32 z = -3; z <= 3; ++z)
        if (x * x + y * y + z * z == r2) out.push_back({x + t, y + t, z + t});
  return out;
}

// 25 des 30 sites de x^2 + y^2 + z^2 = 9 translates de (3, 3, 3) : les cinq derniers non axiaux sont retires.
Points sphere9_25() {
  const Points all = sphere(9, 3);
  Points kept;
  u32 dropped = 0;
  for (auto it = all.rbegin(); it != all.rend(); ++it) {
    const Xyz& p = *it;
    const u32 axial = (p[0] == 3 ? 1u : 0u) + (p[1] == 3 ? 1u : 0u) + (p[2] == 3 ? 1u : 0u);
    if (axial != 2 && dropped < 5) {
      ++dropped;
      continue;
    }
    kept.push_back(p);
  }
  return kept;
}

struct Case {
  Points points;
  Order kmax;
};
std::vector<Case> clouds() {
  return {{{{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}}, 4},
          {{{0, 0, 0}, {2, 2, 0}, {4, 0, 0}, {8, 0, 0}}, 4},
          {{{0, 0, 0}, {2, 2, 0}, {2, 0, 2}}, 3},
          {{{20, 20, 20}, {20, 0, 0}, {0, 20, 0}, {0, 0, 20}, {10, 10, 10}, {11, 10, 10}, {10, 11, 10}}, 5},
          {{{2, 10, 0}, {18, 10, 0}, {10, 20, 0}, {9, 3, 0}, {11, 3, 0}}, 4},
          {{{0, 0, 7}, {0, 9, 6}, {1, 4, 0}, {0, 0, 1}, {4, 1, 2}}, 4},
          {{{0, 0, 0}, {2, 0, 0}, {0, 2, 0}, {2, 2, 0}, {0, 0, 2}, {2, 0, 2}, {0, 2, 2}, {2, 2, 2}}, 5},
          {{{2, 2, 0}, {2, 2, 4}, {0, 2, 2}, {4, 2, 2}, {2, 0, 2}, {2, 4, 2}}, 5},
          {{{0, 0, 0}, {2, 0, 0}, {0, 2, 0}, {2, 2, 0}, {10, 0, 0}, {12, 0, 0}, {11, 2, 0}}, 5},
          {grid(4, 2, 30), 5},
          {grid(5, 3, 40), 4},
          {drawn(40, 6, 3), 5},
          {drawn(70, 16, 5), 5},
          {drawn(90, 64, 7), 5},
          {drawn(60, 8, 13), 6}};
}

struct Built {
  MemoryBudget owner{MemoryBudget::kUnlimited};
  std::optional<OrderTree> tree;
};

FullParams lots() {
  FullParams p;
  p.regular_batch_capacity = 64;
  p.descent_lanes = 4;
  p.reuse_census_workspace = p.dense_birth_lookup = p.population_lookup = true;
  return p;
}

// Arbre d'ordre k ; PointId first, first + step, ... ; voie par lots sur `pool` s'il est donne.
bool build(Built& out, const Points& points, Order k, sched::Pool* pool = nullptr, u32 first = 100, u32 step = 3) {
  auto domain = supports_test::domain_of(points, k, out.owner, first, step);
  if (!domain.ok()) return false;
  auto tree = build_order(std::move(domain.value()), k, out.owner, pool == nullptr ? FullParams{} : lots(), pool);
  if (!tree.ok()) return false;
  out.tree.emplace(std::move(tree.value()));
  return true;
}

std::unique_ptr<sched::Pool> pool_of(u32 workers) {
  auto made = sched::make_pool({workers});
  return made.ok() ? std::move(made.value()) : nullptr;
}

Reason reason_of(const Result<supports::SupportHierarchy>& r) { return r.ok() ? Reason::none : r.outcome().reason; }

}  // namespace

MHGP11_TEST(fixtures, 140) {
  Totals totals;
  u64 orders = 0;
  for (const Case& c : clouds())
    for (Order k = 1; k <= c.kmax && k <= c.points.size(); ++k) {
      Built built;
      REQUIRE(build(built, c.points, k));
      MemoryBudget work(MemoryBudget::kUnlimited);
      {
        auto h = supports::build_support_hierarchy(*built.tree, work);
        REQUIRE(h.ok());
        CHECK_EQ(check(*built.tree, h.value(), totals), std::string());
      }
      CHECK(work.released().ok());
      ++orders;
    }
  std::printf("hierarchy_fixtures ordres=%llu boules=%llu noeuds=%llu naissances=%llu fusions=%llu internes=%llu "
              "supports=%llu etendues=%llu multiples=%llu tetraedres=%llu branches=%llu fermetures=%llu\n",
              static_cast<unsigned long long>(orders), static_cast<unsigned long long>(totals.balls),
              static_cast<unsigned long long>(totals.nodes), static_cast<unsigned long long>(totals.births),
              static_cast<unsigned long long>(totals.merges), static_cast<unsigned long long>(totals.internals),
              static_cast<unsigned long long>(totals.supports), static_cast<unsigned long long>(totals.extended),
              static_cast<unsigned long long>(totals.multiple), static_cast<unsigned long long>(totals.tetra),
              static_cast<unsigned long long>(totals.prior), static_cast<unsigned long long>(totals.closures));
  CHECK(orders >= 66);
  CHECK(totals.balls >= 3000 && totals.extended >= 300 && totals.multiple >= 100 && totals.tetra >= 50);
  CHECK(totals.merges >= 500 && totals.internals >= 200 && totals.closures == totals.balls);
}

MHGP11_TEST(determinism, 60) {
  const auto p1 = pool_of(1), p2 = pool_of(2), p4 = pool_of(4);
  REQUIRE(p1 && p2 && p4);
  u64 compared = 0;
  for (const Case& c : clouds())
    for (Order k = 1; k <= c.kmax && k <= c.points.size(); k = static_cast<Order>(k + 2)) {
      Built serial, batched;
      REQUIRE(build(serial, c.points, k));
      REQUIRE(build(batched, c.points, k, p4.get()));
      MemoryBudget work(MemoryBudget::kUnlimited);
      auto base = supports::build_support_hierarchy(*serial.tree, work);
      REQUIRE(base.ok());
      const u64 want = fingerprint(base.value());
      for (sched::Pool* pool : {p1.get(), p2.get(), p4.get()}) {
        auto other = supports::build_support_hierarchy(*serial.tree, work, pool);
        REQUIRE(other.ok());
        CHECK_EQ(fingerprint(other.value()), want);
        CHECK(other.value().ledger() == base.value().ledger());
      }
      auto lane = supports::build_support_hierarchy(*batched.tree, work, p2.get());
      REQUIRE(lane.ok());
      CHECK_EQ(fingerprint(lane.value()), want);
      ++compared;
    }
  CHECK(compared >= 30);
}

MHGP11_TEST(permutation, 30) {
  u64 compared = 0;
  for (const Case& c : clouds())
    for (Order k : {Order{2}, Order{4}}) {
      if (k > c.kmax || k > c.points.size()) continue;
      Points reversed(c.points.rbegin(), c.points.rend());
      Built straight, permuted, relabeled;
      REQUIRE(build(straight, c.points, k));
      REQUIRE(build(permuted, reversed, k));
      REQUIRE(build(relabeled, c.points, k, nullptr, 0xFFFFFFFFu - 7u * static_cast<u32>(c.points.size()), 7));
      MemoryBudget work(MemoryBudget::kUnlimited);
      auto a = supports::build_support_hierarchy(*straight.tree, work);
      auto b = supports::build_support_hierarchy(*permuted.tree, work);
      auto r = supports::build_support_hierarchy(*relabeled.tree, work);
      REQUIRE(a.ok() && b.ok() && r.ok());
      CHECK_EQ(fingerprint(b.value()), fingerprint(a.value()));
      CHECK_EQ(fingerprint(r.value()), fingerprint(a.value()));
      ++compared;
    }
  CHECK(compared >= 20);
}

// Admission a 24 sites : comptes des auditeurs (receipts/audit_supports_contract_20261005/qb) de K1 a K3.
MHGP11_TEST(sphere5, 30) {
  const Points points = sphere(5, 2);
  REQUIRE(CHECK_EQ(points.size(), 24u));
  const std::array<std::array<u32, 3>, 3> want{{{24, 24, 12}, {276, 264, 288}, {2024, 1736, 3906}}};
  for (Order k = 1; k <= 3; ++k) {
    Built built;
    REQUIRE(build(built, points, k));
    MemoryBudget work(MemoryBudget::kUnlimited);
    auto h = supports::build_support_hierarchy(*built.tree, work);
    REQUIRE(h.ok());
    Totals totals;
    CHECK_EQ(check(*built.tree, h.value(), totals, 0), std::string());
    u32 found = 0;
    for (u64 slot = 0; slot < h.value().balls().size(); ++slot) {
      const auto& ball = h.value().balls()[slot];
      if (ball.m != 24) continue;
      ++found;
      const u64 sb = h.value().support_offsets()[slot], se = h.value().support_offsets()[slot + 1];
      std::array<u32, 5> arity{};
      u64 incidences = 0;
      for (u64 s = sb; s < se; ++s) {
        ++arity[h.value().supports()[s].arity];
        incidences += h.value().support_cofaces()[s];
      }
      CHECK_EQ(se - sb, u64{828});
      CHECK(arity[2] == 12 && arity[3] == 24 && arity[4] == 792);
      CHECK_EQ(ball.kparties_reliees, want[k - 1][0]);
      CHECK_EQ(ball.strict_traces, want[k - 1][1]);
      CHECK_EQ(ball.cofaces, want[k - 1][2]);
      if (k == 3) CHECK_EQ(incidences, u64{4068});
    }
    CHECK_EQ(found, 1u);
  }
}

// Refus de l'appel entier a 25 sites, quel que soit le Pool : rien n'est alloue, diagnostics intacts.
MHGP11_TEST(sphere9, 12) {
  const Points points = sphere9_25();
  REQUIRE(CHECK_EQ(points.size(), 25u));
  const auto p4 = pool_of(4);
  REQUIRE(p4 != nullptr);
  for (Order k : {Order{1}, Order{2}})
    for (sched::Pool* pool : {static_cast<sched::Pool*>(nullptr), p4.get()}) {
      Built built;
      REQUIRE(build(built, points, k));
      MemoryBudget work(MemoryBudget::kUnlimited);
      supports::HierarchyTimings timings{1, 2, 3, 4, 5, {6, 7}};
      const auto saved = timings;
      const auto refused = supports::build_support_hierarchy(*built.tree, work, pool, &timings);
      CHECK_EQ(reason_of(refused), Reason::support_shell_capacity);
      CHECK(work.peak() == 0 && work.used() == 0 && timings == saved);
    }
}

// Formule d'admission exacte, refus avant toute allocation, second etage.
MHGP11_TEST(admission, 60) {
  const auto p2 = pool_of(2), p4 = pool_of(4);
  REQUIRE(p2 && p4);
  const std::vector<std::pair<Points, Order>> cases{{sphere(5, 2), 2}, {clouds()[0].points, 2}, {drawn(60, 8, 13), 4}};
  for (const auto& [points, k] : cases)
    for (sched::Pool* pool : {static_cast<sched::Pool*>(nullptr), p2.get(), p4.get()}) {
      Built built;
      REQUIRE(build(built, points, k));
      const Admission want = admission(*built.tree, pool == nullptr ? 1 : pool->size());
      u64 second = 0;
      {
        MemoryBudget work(MemoryBudget::kUnlimited);
        supports::HierarchyTimings timings;
        auto h = supports::build_support_hierarchy(*built.tree, work, pool, &timings);
        REQUIRE(h.ok());
        second = (sizeof(supports::Support) + 4) * h.value().supports().size();
        CHECK_EQ(timings.admitted[0], want.first);
        CHECK_EQ(timings.admitted[1], second);
        CHECK_EQ(work.peak(), want.first + second);
        CHECK_EQ(work.used(), want.retained + second);
      }
      for (u64 limit : {want.first - 1, want.first + second - 1, want.first + second}) {
        MemoryBudget work(limit);
        supports::HierarchyTimings timings{1, 2, 3, 4, 5, {6, 7}};
        const auto saved = timings;
        {
          auto h = supports::build_support_hierarchy(*built.tree, work, pool, &timings);
          if (limit == want.first + second) {
            CHECK(h.ok());
            continue;
          }
          CHECK_EQ(reason_of(h), Reason::memory_budget);
          CHECK(timings == saved);
        }
        CHECK_EQ(work.used(), u64{0});
        if (limit == want.first - 1) CHECK_EQ(work.peak(), u64{0});
      }
    }
}

MHGP11_TEST_MAIN()
