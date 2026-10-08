// Census garde d'une boule certifiee (NUM-CERTIFIEE, NUM-GARDE ; CST-0108, CST-0109, CST-0201) : memes populations
// que le census generique de la meme sphere, possede et emprunte ; temoins exacts du contrat et de l'auditeur.
#include <algorithm>
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

// Boite partielle qui contient tout le support et le centre (temoin de l'auditeur) : raffinee, jamais rejetee ; temoin
// des certificats lies a leur domaine (CST-0201) dans un census : support aigu d'etendue 21 (h = 2^21 - 1, tout profil)
// et site (h,h,h) dans le pave resserre (premier produit a 131 bits, essai controle en echec, repli large) ; ancien coin
// (3M-1)^3 d'etendue 20 (B >= 22) : hors du pave resserre, rejete sans arithmetique, meme census.
MHGP12_TEST(witnesses, 15) {
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
  {
    const u32 h = (u32{1} << 21) - 1;
    const auto ball = certify({point(0, 0, 0), point(h, h, 0), point(h, 0, h)});
    REQUIRE(ball.has_value());
    const Input far({{0, 0, 0}, {h, h, 0}, {h, 0, h}, {h, h, h}});
    auto far_cloud = far.prepare(budget);
    REQUIRE(far_cloud.ok());
    auto far_index = build_index(std::move(far_cloud.value()), {1}, budget);
    REQUIRE(far_index.ok());
    auto g = census(far_index.value(), *ball, kNone, budget);
    auto e = census(far_index.value(), ball->sphere(), kNone, budget);
    REQUIRE(g.ok() && e.ok());
    CHECK(same(g.value(), e.value()));
    CHECK_EQ(g.value().shell().size(), 3u);      // le support ; (h,h,h) est dehors (puissance 2 h^6)
    CHECK(g.value().ledger().lanes.wide >= 2u);  // le site (h,h,h), a chaque passe
  }
  if constexpr (kCoordBits >= 22) {
    // Support aigu d'etendue 20, site (3M-1)^3 au coin de l'ANCIEN pave : hors du pave resserre, sans arithmetique.
    // Feuilles d'un site : la feuille du coin est une boite disjointe du pave, rejetee avant tout test de site (au
    // profil 24 : guard_disjoint = 2, guard_outside = 0, aucune voie large) ; avec l'ancien pave elle y entrait.
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
    CHECK(g.value().ledger().guard_disjoint >= 2u);  // la feuille du coin, a chaque passe
    CHECK_EQ(g.value().ledger().lanes.wide, 0u);
  } else {
    CHECK(kCoordBits < 22);
  }
}

// Census a temoins sur la sphere (T2-d) : un noeud qui contient un site du support est raffine sans bornes evaluees.
// Contre le census garde sans temoins, a plusieurs seuils et tailles de feuille, sur des nuages graves : memes resultats,
// memes noeuds et memes sites testes (les temoins sont sur la sphere), decisions par temoin et moins d'evaluations ; un
// faux temoin (site interieur ou exterieur) garde I et U ; refus au-dela de quatre temoins ou hors du nuage.
namespace {
struct WitnessTotals { u64 queries = 0, witness = 0, plain_lanes = 0, witness_lanes = 0, false_queries = 0; };

// SiteIdx du site de coordonnees p dans le Cloud (trie par cle de Morton), kNone s'il n'y est pas.
u32 site_of(const Cloud& cloud, std::array<u32, 3> p) {
  for (u32 i = 0; i < cloud.sites(); ++i)
    if (cloud.x()[i] == p[0] && cloud.y()[i] == p[1] && cloud.z()[i] == p[2]) return i;
  return kNone;
}

void judge_witnesses(const Input& input, const std::vector<std::array<u32, 3>>& support, WitnessTotals& totals) {
  std::vector<num::Point> pts;
  for (const auto& s : support) pts.push_back(point(s[0], s[1], s[2]));
  const auto made = num::CertifiedBall::certify(pts);
  REQUIRE(made.ok() && made.value());
  const num::CertifiedBall& ball = *made.value();
  for (u32 leaf : {1u, 3u, 8u}) {
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
    auto cloud = input.prepare(owner);
    REQUIRE(cloud.ok());
    auto index = build_index(std::move(cloud.value()), {leaf}, owner);
    REQUIRE(index.ok());
    auto workspace = CensusWorkspace::make(index.value(), work);
    REQUIRE(workspace.ok());
    std::vector<SiteIdx> witnesses;
    for (const auto& s : support) witnesses.push_back(make_id<SiteIdx>(site_of(index.value().cloud(), s)));
    for (const SiteIdx w : witnesses) REQUIRE(idx(w) != kNone);
    for (u32 threshold : {1u, 2u, 3u, 5u, kNone}) {
      Borrowed plain, seen;
      REQUIRE(workspace.value()->query(index.value(), ball, threshold, &plain, Borrowed::call).ok());
      REQUIRE(workspace.value()->query(index.value(), ball, threshold, witnesses, &seen, Borrowed::call).ok());
      CHECK(plain.kind == seen.kind && plain.interior == seen.interior && plain.shell == seen.shell);
      CHECK_EQ(seen.ledger.nodes, plain.ledger.nodes);
      CHECK_EQ(seen.ledger.point_tests, plain.ledger.point_tests);
      CHECK_EQ(seen.ledger.bounds, plain.ledger.bounds);
      CHECK_EQ(plain.ledger.guard_witness, 0u);
      CHECK(seen.ledger.lanes.total() <= plain.ledger.lanes.total());
      ++totals.queries;
      totals.witness += seen.ledger.guard_witness;
      totals.plain_lanes += plain.ledger.lanes.total();
      totals.witness_lanes += seen.ledger.lanes.total();
      // Faux temoins : chaque site du nuage hors du support ; memes I et U, seul le travail peut changer.
      for (u32 site = 0; site < index.value().cloud().sites(); site += 3) {
        if (std::find(witnesses.begin(), witnesses.end(), make_id<SiteIdx>(site)) != witnesses.end()) continue;
        const SiteIdx wrong[1] = {make_id<SiteIdx>(site)};
        Borrowed other;
        REQUIRE(workspace.value()->query(index.value(), ball, threshold, wrong, &other, Borrowed::call).ok());
        CHECK(plain.kind == other.kind && plain.interior == other.interior && plain.shell == other.shell);
        ++totals.false_queries;
      }
    }
  }
}
}  // namespace

MHGP12_TEST(witness_census, 2000) {
  WitnessTotals totals;
  const Input dense = octa();
  for (const auto& support : std::vector<std::vector<std::array<u32, 3>>>{
           {{2, 4, 4}, {6, 4, 4}}, {{0, 0, 0}, {8, 8, 8}}, {{4, 4, 2}, {4, 4, 6}},
           {{2, 4, 4}, {4, 2, 4}, {4, 4, 6}}})
    judge_witnesses(dense, support, totals);
  std::vector<std::array<u32, 3>> spread;  // nuage etale : boites disjointes, partielles et contenues
  for (u32 x = 0; x < 7; ++x)
    for (u32 y = 0; y < 7; ++y) spread.push_back({x * 37 % 101, y * 53 % 97, (x * y * 29 + x) % 89});
  const Input wide(spread);
  for (const auto& support : std::vector<std::vector<std::array<u32, 3>>>{
           {spread[0], spread[48]}, {spread[8], spread[30]}, {spread[3], spread[17]}})
    judge_witnesses(wide, support, totals);
  std::printf("witness census queries=%llu witness=%llu lanes_plain=%llu lanes_witness=%llu false=%llu\n",
              static_cast<unsigned long long>(totals.queries), static_cast<unsigned long long>(totals.witness),
              static_cast<unsigned long long>(totals.plain_lanes), static_cast<unsigned long long>(totals.witness_lanes),
              static_cast<unsigned long long>(totals.false_queries));
  CHECK(totals.queries >= 100 && totals.witness > 0 && totals.false_queries >= 300);
  CHECK(totals.witness_lanes < totals.plain_lanes);
  // Refus : plus de quatre temoins, temoin hors du nuage ; le workspace reste utilisable.
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto cloud = dense.prepare(budget);
  REQUIRE(cloud.ok());
  auto index = build_index(std::move(cloud.value()), {2}, budget);
  REQUIRE(index.ok());
  auto workspace = CensusWorkspace::make(index.value(), budget);
  REQUIRE(workspace.ok());
  const auto made = num::CertifiedBall::certify(std::vector<num::Point>{point(2, 4, 4), point(6, 4, 4)});
  REQUIRE(made.ok() && made.value());
  const SiteIdx five[5] = {SiteIdx{0}, SiteIdx{1}, SiteIdx{2}, SiteIdx{3}, SiteIdx{4}};
  Borrowed sink;
  CHECK_EQ(workspace.value()->query(index.value(), *made.value(), 2, five, &sink, Borrowed::call).reason,
           Reason::parameter_out_of_range);
  const SiteIdx outside[1] = {SiteIdx{index.value().cloud().sites()}};
  CHECK_EQ(workspace.value()->query(index.value(), *made.value(), 2, outside, &sink, Borrowed::call).reason,
           Reason::parameter_out_of_range);
  CHECK(workspace.value()->query(index.value(), *made.value(), 2, std::span<const SiteIdx>(five, 1), &sink,
                                 Borrowed::call).ok());
}

MHGP12_TEST_MAIN()
