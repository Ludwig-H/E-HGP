// Census garde d'une boule certifiee (NUM-CERTIFIEE, NUM-GARDE ; CST-0108, CST-0109, CST-0201) : memes populations
// que le census generique de la meme sphere, possede et emprunte ; temoins exacts du contrat et de l'auditeur.
#include <vector>

#include "test_support.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace index_test;

namespace {
std::optional<num::CertifiedBall> certify(std::initializer_list<num::Point> support) {
  const std::vector<num::Point> points(support);
  const auto made = num::CertifiedBall::certify(points);
  if (!made.ok()) throw std::runtime_error("certificat de test refuse");
  return made.value();
}
struct Borrowed {
  CensusKind kind = CensusKind::complete;
  std::vector<SiteIdx> interior, shell;
  CensusLedger ledger;
  static Outcome call(void* raw, const BorrowedCensus& result) {
    auto& b = *static_cast<Borrowed*>(raw);
    b.kind = result.kind();
    b.interior = copy(result.interior());
    b.shell = copy(result.shell());
    b.ledger = result.ledger();
    return {};
  }
};
bool same(const Census& a, const Census& b) {
  return a.kind() == b.kind() && equal(a.interior(), b.interior()) && equal(a.shell(), b.shell());
}
bool same(const Census& a, const Borrowed& b) {
  return a.kind() == b.kind && copy(a.interior()) == b.interior && copy(a.shell()) == b.shell;
}
struct Totals { u64 queries = 0, disjoint = 0, partial = 0, outside = 0, wide = 0, borrowed = 0; };

// Toutes les boules certifiables d'un support, contre le census generique, a plusieurs seuils et tailles de feuille.
void judge(const Input& input, const num::CertifiedBall& ball, Totals& totals) {
  for (u32 leaf : {1u, 4u, 16u}) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited), query(MemoryBudget::kUnlimited);
    auto cloud = input.prepare(owner);
    REQUIRE(cloud.ok());
    auto index = build_index(std::move(cloud.value()), {leaf}, owner);
    REQUIRE(index.ok());
    auto workspace = CensusWorkspace::make(index.value(), work);
    REQUIRE(workspace.ok());
    for (u32 threshold : {1u, 2u, 3u, kNone}) {
      auto generic = census(index.value(), ball.sphere(), threshold, query);
      auto guarded = census(index.value(), ball, threshold, query);
      REQUIRE(generic.ok() && guarded.ok());
      CHECK(same(generic.value(), guarded.value()));
      Borrowed borrowed;
      REQUIRE(workspace.value()->query(index.value(), ball, threshold, &borrowed, Borrowed::call).ok());
      CHECK(same(guarded.value(), borrowed));
      totals.borrowed += borrowed.ledger.guard_disjoint + borrowed.ledger.guard_partial + borrowed.ledger.guard_outside;
      const auto& l = guarded.value().ledger();
      CHECK_EQ(l.passes, 2u);
      CHECK_EQ(generic.value().ledger().guard_disjoint + generic.value().ledger().guard_partial +
               generic.value().ledger().guard_outside, 0u);
      ++totals.queries;
      totals.disjoint += l.guard_disjoint;
      totals.partial += l.guard_partial;
      totals.outside += l.guard_outside;
      totals.wide += l.lanes.wide;
    }
  }
}
}  // namespace

MHGP12_TEST(fixtures, 400) {
  Totals totals;
  const Input dense = octa();
  // Supports pris dans le nuage et hors du nuage, des quatre arites, dont la paire (2,4,4)-(6,4,4) de ball().
  for (const auto& support : std::vector<std::vector<num::Point>>{
           {point(4, 4, 4)}, {point(2, 4, 4), point(6, 4, 4)}, {point(0, 0, 0), point(8, 8, 8)},
           {point(2, 4, 4), point(4, 2, 4), point(4, 4, 6)}, {point(1, 1, 1), point(7, 2, 1), point(3, 6, 2)},
           {point(0, 0, 0), point(4, 4, 0), point(4, 0, 4), point(0, 4, 4)},
           {point(2, 2, 2), point(6, 6, 2), point(6, 2, 6), point(2, 6, 6)}}) {
    const auto made = num::CertifiedBall::certify(support);
    REQUIRE(made.ok() && made.value());
    judge(dense, *made.value(), totals);
  }
  // Nuage etale : boites disjointes du pave et sites hors du pave.
  std::vector<std::array<u32, 3>> spread;
  for (u32 x = 0; x < 6; ++x)
    for (u32 y = 0; y < 6; ++y) spread.push_back({x * 37 % 101, y * 53 % 97, (x * y * 29) % 89});
  const Input wide(spread);
  for (const auto& support : std::vector<std::vector<num::Point>>{
           {point(37, 53, 29)}, {point(30, 40, 20), point(44, 60, 30)},
           {point(20, 20, 20), point(60, 25, 22), point(35, 60, 40)}}) {
    const auto made = num::CertifiedBall::certify(support);
    REQUIRE(made.ok() && made.value());
    judge(wide, *made.value(), totals);
  }
  std::printf("guarded fixtures queries=%llu disjoint=%llu partial=%llu outside=%llu wide=%llu\n",
              static_cast<unsigned long long>(totals.queries), static_cast<unsigned long long>(totals.disjoint),
              static_cast<unsigned long long>(totals.partial), static_cast<unsigned long long>(totals.outside),
              static_cast<unsigned long long>(totals.wide));
  CHECK(totals.queries >= 100 && totals.disjoint > 0 && totals.partial > 0 && totals.outside > 0 && totals.borrowed > 0);
}

// Candidates non certifiees (CST-0108) : elles n'ont pas le type de la garde et restent dans le census generique, qui
// trouve les sites lointains de leur boule ; le pave de leur support les aurait perdus.
MHGP12_TEST(uncertified, 12) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  // Triangle presque aligne : centre (1100, 1001 - 5000.5, 0), rayon 5000.5 ; (1100, 0, 0) est dans la boule, loin
  // sous le pave (488, 1768) du support.
  const auto a = point(1000, 1000, 0), b = point(1100, 1001, 0), c = point(1200, 1000, 0);
  CHECK(!certify({a, b, c}).has_value());
  const Input flat({{1000, 1000, 0}, {1100, 1001, 0}, {1200, 1000, 0}, {1100, 0, 0}, {1100, 900, 0}, {5000, 5000, 0}});
  auto cloud = flat.prepare(budget);
  REQUIRE(cloud.ok());
  auto index = build_index(std::move(cloud.value()), {1}, budget);
  REQUIRE(index.ok());
  const auto made = num::Sphere::through(a, b, c);
  REQUIRE(made.ok() && made.value());
  auto generic = census(index.value(), *made.value(), kNone, budget);
  REQUIRE(generic.ok());
  CHECK_EQ(generic.value().interior().size(), 2u);  // (1100,0,0) et (1100,900,0)
  CHECK_EQ(generic.value().shell().size(), 3u);     // le support
  // Temoin de l'auditeur : (0,479,0) sur la sphere de S={(419,0,0),(435,15,0),(434,14,0)}, hors du pave.
  const auto p = point(419, 0, 0), q = point(435, 15, 0), r = point(434, 14, 0);
  CHECK(!certify({p, q, r}).has_value());
  const Input codex({{419, 0, 0}, {435, 15, 0}, {434, 14, 0}, {0, 479, 0}, {600, 600, 0}});
  auto codex_cloud = codex.prepare(budget);
  REQUIRE(codex_cloud.ok());
  auto codex_index = build_index(std::move(codex_cloud.value()), {1}, budget);
  REQUIRE(codex_index.ok());
  const auto candidate = num::Sphere::through(p, q, r);
  REQUIRE(candidate.ok() && candidate.value());
  auto shell = census(codex_index.value(), *candidate.value(), kNone, budget);
  REQUIRE(shell.ok());
  CHECK_EQ(shell.value().shell().size(), 4u);  // les trois sites du support et (0,479,0)
}

// Boite partielle qui contient tout le support et le centre (temoin de l'auditeur) : raffinee, jamais rejetee ; et
// temoin des certificats lies a leur domaine (CST-0201) dans un census, des que le profil admet la requete (B >= 22).
MHGP12_TEST(witnesses, 9) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const auto made = certify({point(100, 100, 100), point(102, 100, 100)});
  REQUIRE(made.has_value());
  const Input partial({{0, 0, 0}, {200, 200, 200}, {100, 100, 100}, {102, 100, 100}, {101, 100, 100},
                       {101, 102, 100}, {150, 20, 180}});
  auto cloud = partial.prepare(budget);
  REQUIRE(cloud.ok());
  auto index = build_index(std::move(cloud.value()), {2}, budget);
  REQUIRE(index.ok());
  auto guarded = census(index.value(), *made, kNone, budget);
  auto generic = census(index.value(), made->sphere(), kNone, budget);
  REQUIRE(guarded.ok() && generic.ok());
  CHECK(same(guarded.value(), generic.value()));
  CHECK_EQ(guarded.value().interior().size(), 1u);  // (101,100,100)
  CHECK_EQ(guarded.value().shell().size(), 2u);
  CHECK(guarded.value().ledger().guard_partial >= 2u);  // la racine [0,200]^3 dans chacune des deux passes
  if constexpr (kCoordBits >= 22) {
    // Support aigu d'etendue 20, site (3M-1)^3 au coin du pave : premier produit au-dela de 2^127, voie large.
    const i64 m = i64{1} << 20, h = m - 1;
    const auto ball = certify({point(0, 0, 0), point(h, h, 0), point(h, 0, h)});
    REQUIRE(ball.has_value());
    const u32 corner = static_cast<u32>(3 * m - 1);
    const Input far({{0, 0, 0}, {static_cast<u32>(h), static_cast<u32>(h), 0}, {corner, corner, corner}});
    auto far_cloud = far.prepare(budget);
    REQUIRE(far_cloud.ok());
    auto far_index = build_index(std::move(far_cloud.value()), {1}, budget);
    REQUIRE(far_index.ok());
    auto g = census(far_index.value(), *ball, kNone, budget);
    auto e = census(far_index.value(), ball->sphere(), kNone, budget);
    REQUIRE(g.ok() && e.ok());
    CHECK(same(g.value(), e.value()));
    CHECK(g.value().ledger().lanes.wide >= 2u);  // le site du coin, a chaque passe
  } else {
    CHECK(kCoordBits < 22);
  }
}

MHGP12_TEST_MAIN()
