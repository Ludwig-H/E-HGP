// Portes unitaires du module supports : Q_b et comptes du lemme G sur les fixtures gravees de la specification de la
// sortie parametree (paragraphe 2.9 : 1 carre, 2 triangle droit, 3 growth_ABCZ, 9 cube, 10 octaedre, 11 cercle a B_t,
// 12 lignes et losanges), la coquille mixte de l'audit de4ab58a8 (une paire, 4 triangles, 13 tetraedres), les bornes
// du plafond (24 sites cocycliques admis, 25 refuses) et les refus de l'API. Les boules sont designees par leur S*.
// Attendus calcules par la DEFINITION en Fraction (boules minimales de toutes les parties de la fixture, brouillon
// hors depot de l'auteur), puis recoupes par les formules du lemme G : aucun attendu n'est lu dans le produit.
#include <thread>
#include <vector>

#include "supports_support.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace supports_test;
using supports::BallCounts;
using supports::Support;

namespace {

struct Expected {
  std::vector<Points> supports;  // ordre publie, S* en tete
  std::vector<u32> closure;      // N_0 .. N_m
};
struct AtOrder {
  Order k;
  BallCounts ball;                  // kparties_reliees, compressed_parts, strict_traces, cofaces, gabriel_cofaces
  std::vector<u32> cofaces;         // support_cofaces, dans l'ordre des supports
  std::vector<u32> gabriel;         // support_gabriel_cofaces
};

struct Run {
  std::vector<Support> out = std::vector<Support>(supports::kMaxSupports);
  std::vector<u64> scratch = std::vector<u64>(supports::kMaxClosureWords);
  supports::SupportLedger ledger;
};

// Q_b, fermeture et comptes d'une boule designee par S*. Controles : 4 + |Q_b| + (m+1) + 2 par ordre + 2 |Q_b| par ordre.
void check_ball(const FullDomain& domain, const Points& star, const Expected& want, const std::vector<AtOrder>& orders,
                Run& run) {
  const auto ball = ball_of(domain, star);
  REQUIRE(ball.has_value());
  const auto made = supports::ball_supports(domain, *ball, run.out, run.scratch, &run.ledger);
  REQUIRE(made.ok());
  const auto found = std::span<const Support>(run.out).first(made.value().count);
  REQUIRE(CHECK_EQ(found.size(), want.supports.size()));
  CHECK(published_order(found));
  const auto& data = domain.catalogue().balls_data()[idx(*ball)];
  CHECK(found[0] == (Support{data.support, data.qmin}));
  const auto sites = coordinates(domain, found);
  for (std::size_t i = 0; i < sites.size(); ++i) CHECK(sites[i] == want.supports[i]);
  const auto& closure = made.value().closure;
  REQUIRE(CHECK_EQ(closure.shell() + 1, want.closure.size()));
  for (u32 j = 0; j < want.closure.size(); ++j) CHECK_EQ(closure.parts(j), want.closure[j]);
  for (const AtOrder& at : orders) {
    const auto shape = supports::ball_shape(domain, *ball, at.k);
    REQUIRE(shape.ok());
    const auto counts = supports::ball_counts(shape.value(), closure);
    REQUIRE(counts.ok());
    CHECK(counts.value() == at.ball);
    if (!(counts.value() == at.ball))
      std::printf("      K%u : obtenu %u %u %u %u %u\n", unsigned{at.k}, counts.value().kparties_reliees,
                  counts.value().compressed_parts, counts.value().strict_traces, counts.value().cofaces,
                  counts.value().gabriel_cofaces);
    REQUIRE(at.cofaces.size() == found.size() && at.gabriel.size() == found.size());
    for (std::size_t i = 0; i < found.size(); ++i) {
      CHECK_EQ(supports::support_cofaces(shape.value(), found[i].arity), at.cofaces[i]);
      CHECK_EQ(supports::support_gabriel_cofaces(shape.value(), found[i].arity), at.gabriel[i]);
    }
  }
}

Points square() { return {{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}}; }
Points cube() {
  Points points;
  for (u32 x : {0u, 2u})
    for (u32 y : {0u, 2u})
      for (u32 z : {0u, 2u}) points.push_back({x, y, z});
  return points;
}
// Coquille de rayon 5 de l'audit de4ab58a8 (receipts/audit_supports_followup_20261004/qb), translatee de +5.
Points mixed(u32 scale) {
  const Points base = {{10, 5, 5}, {2, 9, 5}, {2, 1, 5}, {8, 5, 9}, {2, 5, 9}, {5, 8, 1}, {5, 2, 1}, {5, 5, 10}, {5, 5, 0}};
  Points out;
  for (Xyz p : base) out.push_back({p[0] * scale, p[1] * scale, p[2] * scale});
  return out;
}
// Les 25 premiers sites entiers de x^2 + y^2 = 65^2, (65, 0) et (-65, 0) en tete puis par angle, translates en
// (66, 66, 7) : fixture cercle24/cercle25 de tests/catalogue/euler_limits.py.
Points cocircular(std::size_t count) {
  const std::array<std::array<u32, 2>, 25> xy = {{{131, 66}, {1, 66},   {3, 50},   {6, 41},   {10, 33},
                                                  {14, 27},  {27, 14},  {33, 10},  {41, 6},   {50, 3},
                                                  {66, 1},   {82, 3},   {91, 6},   {99, 10},  {105, 14},
                                                  {118, 27}, {122, 33}, {126, 41}, {129, 50}, {129, 82},
                                                  {126, 91}, {122, 99}, {118, 105}, {105, 118}, {99, 122}}};
  Points out;
  for (std::size_t i = 0; i < count; ++i) out.push_back({xy[i][0], xy[i][1], 7});
  return out;
}

}  // namespace

// Fixture 1. K1 : diagonales 1 coface chacune ; K2 : 4 traces strictes, 4 cofaces, 2 par diagonale ; K3 : naissance
// etendue (0 trace stricte), 1 coface comptee par les deux diagonales ; K4 : naissance, 0 coface.
MHGP11_TEST(square, 79) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto domain = domain_of(square(), 4, budget);
  REQUIRE(domain.ok());
  Run run;
  check_ball(domain.value(), {{0, 0, 0}, {2, 2, 0}},
             {{{{0, 0, 0}, {2, 2, 0}}, {{2, 0, 0}, {0, 2, 0}}}, {0, 0, 2, 4, 1}},
             {{1, {4, 4, 4, 2, 2}, {1, 1}, {1, 1}},
              {2, {6, 6, 4, 4, 4}, {2, 2}, {2, 2}},
              {3, {4, 4, 0, 1, 1}, {1, 1}, {1, 1}},
              {4, {1, 1, 0, 0, 0}, {0, 0}, {0, 0}}},
             run);
  check_ball(domain.value(), {{0, 0, 0}, {2, 0, 0}}, {{{{0, 0, 0}, {2, 0, 0}}}, {0, 0, 1}},
             {{1, {2, 2, 2, 1, 1}, {1}, {1}}, {2, {1, 1, 0, 0, 0}, {0}, {0}}, {4, {0, 0, 0, 0, 0}, {0}, {0}}}, run);
  // Une coquille etendue (sphere, 6 paires, 4 triplets, 1 quadruplet) et une reguliere (aucun predicat).
  CHECK(run.ledger == (supports::SupportLedger{2, 1, 1, 3, 6, 4, 0, 1}));
}

// Fixture 2. L'hypotenuse est le seul support : le triangle droit n'en est pas un, l'origine est un site orpheline.
MHGP11_TEST(right_triangle, 51) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto domain = domain_of({{0, 0, 0}, {4, 0, 0}, {0, 3, 0}}, 2, budget);
  REQUIRE(domain.ok());
  Run run;
  check_ball(domain.value(), {{0, 3, 0}, {4, 0, 0}}, {{{{0, 3, 0}, {4, 0, 0}}}, {0, 0, 1, 1}},
             {{1, {3, 3, 3, 1, 1}, {1}, {1}}, {2, {3, 3, 2, 1, 1}, {1}, {1}}}, run);
  check_ball(domain.value(), {{0, 0, 0}, {0, 3, 0}}, {{{{0, 0, 0}, {0, 3, 0}}}, {0, 0, 1}},
             {{1, {2, 2, 2, 1, 1}, {1}, {1}}, {2, {1, 1, 0, 0, 0}, {0}, {0}}}, run);
  // Coquille de l'hypotenuse : trois milieux, un angle (triangle droit : aucune orientation), aucun quadruplet.
  CHECK(run.ledger == (supports::SupportLedger{2, 1, 1, 2, 3, 1, 0, 0}));
}

// Fixture 3 (growth_ABCZ, K3) : la boule de niveau 25 porte BZ et le triangle aigu ACZ ; une trace stricte (ABC),
// une coface, une par support. La boule AC (naissance ABC au niveau 16) est reguliere.
MHGP11_TEST(growth, 42) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto domain = domain_of({{1, 8, 0}, {5, 10, 0}, {9, 8, 0}, {5, 0, 0}}, 3, budget);
  REQUIRE(domain.ok());
  Run run;
  check_ball(domain.value(), {{5, 0, 0}, {5, 10, 0}},
             {{{{5, 0, 0}, {5, 10, 0}}, {{5, 0, 0}, {1, 8, 0}, {9, 8, 0}}}, {0, 0, 1, 3, 1}},
             {{3, {4, 4, 1, 1, 1}, {1, 1}, {1, 1}}}, run);
  check_ball(domain.value(), {{1, 8, 0}, {9, 8, 0}}, {{{{1, 8, 0}, {9, 8, 0}}}, {0, 0, 1}},
             {{3, {1, 1, 0, 0, 0}, {0}, {0}}}, run);
}

// Fixture 9. Quatre diametres et deux tetraedres, aucun triangle ; six supports (pas les 177 parties de la fermeture,
// audit de4ab58a8) ; a K1 les tetraedres restent dans Q_b avec zero coface. Une face carree. Permutation de l'entree et
// reetiquetage des PointId : memes SiteIdx, memes supports, meme fermeture.
MHGP11_TEST(cube, 110) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto domain = domain_of(cube(), 2, budget);
  REQUIRE(domain.ok());
  Run run;
  const Points star = {{0, 0, 0}, {2, 2, 2}};
  const Expected big = {{{{0, 0, 0}, {2, 2, 2}},
                         {{2, 0, 0}, {0, 2, 2}},
                         {{0, 2, 0}, {2, 0, 2}},
                         {{2, 2, 0}, {0, 0, 2}},
                         {{0, 0, 0}, {2, 2, 0}, {2, 0, 2}, {0, 2, 2}},
                         {{2, 0, 0}, {0, 2, 0}, {0, 0, 2}, {2, 2, 2}}},
                        {0, 0, 4, 24, 56, 56, 28, 8, 1}};
  check_ball(domain.value(), star, big,
             {{1, {8, 8, 8, 4, 4}, {1, 1, 1, 1, 0, 0}, {1, 1, 1, 1, 0, 0}},
              {2, {28, 28, 24, 24, 24}, {6, 6, 6, 6, 0, 0}, {6, 6, 6, 6, 0, 0}}},
             run);
  // C(8,2) milieux, C(8,3) angles, 8 triangles equilateraux orientes (aucun coplanaire au centre), C(8,4) tetraedres.
  CHECK(run.ledger == (supports::SupportLedger{1, 0, 1, 6, 28, 56, 8, 70}));
  u32 closed = 0;
  for (u32 j = 0; j <= 8; ++j) closed += big.closure[j];
  CHECK_EQ(closed, 177u);
  check_ball(domain.value(), {{0, 0, 0}, {2, 2, 0}}, {{{{0, 0, 0}, {2, 2, 0}}, {{2, 0, 0}, {0, 2, 0}}}, {0, 0, 2, 4, 1}},
             {{1, {4, 4, 4, 2, 2}, {1, 1}, {1, 1}}, {2, {6, 6, 4, 4, 4}, {2, 2}, {2, 2}}}, run);
  Points permuted = cube();
  std::reverse(permuted.begin(), permuted.end());
  std::swap(permuted[1], permuted[5]);
  auto other = domain_of(permuted, 2, budget, 0xFFFFFFF0u, 1);
  REQUIRE(other.ok());
  const auto a = ball_of(domain.value(), star), b = ball_of(other.value(), star);
  REQUIRE(a && b);
  CHECK_EQ(idx(*a), idx(*b));
  Run again;
  const auto first = supports::ball_supports(domain.value(), *a, run.out, run.scratch);
  const auto second = supports::ball_supports(other.value(), *b, again.out, again.scratch);
  REQUIRE(first.ok() && second.ok());
  REQUIRE(CHECK_EQ(first.value().count, second.value().count));
  for (u32 i = 0; i < first.value().count; ++i) CHECK(run.out[i] == again.out[i]);
  for (u32 j = 0; j <= 8; ++j) CHECK_EQ(first.value().closure.parts(j), second.value().closure.parts(j));
}

// Fixture 10 : trois paires antipodales ; ni triangle (faces hors du centre, ou triangles droits) ni tetraedre.
MHGP11_TEST(octahedron, 29) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto domain = domain_of({{0, 1, 1}, {2, 1, 1}, {1, 0, 1}, {1, 2, 1}, {1, 1, 0}, {1, 1, 2}}, 2, budget);
  REQUIRE(domain.ok());
  Run run;
  check_ball(domain.value(), {{1, 1, 0}, {1, 1, 2}},
             {{{{1, 1, 0}, {1, 1, 2}}, {{1, 0, 1}, {1, 2, 1}}, {{0, 1, 1}, {2, 1, 1}}}, {0, 0, 3, 12, 15, 6, 1}},
             {{2, {15, 15, 12, 12, 12}, {4, 4, 4}, {4, 4, 4}}}, run);
}

// Fixture 11 (audit 1bf4be68f, carrier) : sur le cercle a quatre points cardinaux, deux diametres ; B remplace par
// B_t (t = 1/n) : un diametre et le triangle strict B_tCD. Immersions entieres exactes n = 4 et n = 255 (u18).
MHGP11_TEST(circle, 72) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  Run run;
  auto base = domain_of({{34, 17, 0}, {17, 34, 0}, {0, 17, 0}, {17, 0, 0}}, 2, budget);
  REQUIRE(base.ok());
  check_ball(base.value(), {{17, 0, 0}, {17, 34, 0}}, {{{{17, 0, 0}, {17, 34, 0}}, {{0, 17, 0}, {34, 17, 0}}}, {0, 0, 2, 4, 1}},
             {{2, {6, 6, 4, 4, 4}, {2, 2}, {2, 2}}}, run);
  auto moved = domain_of({{34, 17, 0}, {25, 32, 0}, {0, 17, 0}, {17, 0, 0}}, 2, budget);
  REQUIRE(moved.ok());
  check_ball(moved.value(), {{0, 17, 0}, {34, 17, 0}},
             {{{{0, 17, 0}, {34, 17, 0}}, {{17, 0, 0}, {0, 17, 0}, {25, 32, 0}}}, {0, 0, 1, 3, 1}},
             {{2, {6, 6, 5, 3, 3}, {2, 1}, {2, 1}}}, run);
  auto wide = domain_of({{130052, 65026, 0}, {65536, 130050, 0}, {0, 65026, 0}, {65026, 0, 0}}, 2, budget);
  REQUIRE(wide.ok());
  check_ball(wide.value(), {{0, 65026, 0}, {130052, 65026, 0}},
             {{{{0, 65026, 0}, {130052, 65026, 0}}, {{65026, 0, 0}, {0, 65026, 0}, {65536, 130050, 0}}}, {0, 0, 1, 3, 1}},
             {{2, {6, 6, 5, 3, 3}, {2, 1}, {2, 1}}}, run);
}

// Fixture 12 : ligne 0,4,6,8,12 (K2 : la boule de niveau 4 a un interieur), deux losanges (K3 : trois naissances
// etendues m = 4, et la boule de niveau 52 a deux interieurs et deux diametres), ligne 0,2,4 (K2).
MHGP11_TEST(lines, 149) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  Run run;
  auto line = domain_of({{0, 0, 0}, {4, 0, 0}, {6, 0, 0}, {8, 0, 0}, {12, 0, 0}}, 2, budget);
  REQUIRE(line.ok());
  check_ball(line.value(), {{4, 0, 0}, {8, 0, 0}}, {{{{4, 0, 0}, {8, 0, 0}}}, {0, 0, 1}},
             {{2, {3, 2, 2, 1, 1}, {1}, {1}}}, run);
  check_ball(line.value(), {{0, 0, 0}, {4, 0, 0}}, {{{{0, 0, 0}, {4, 0, 0}}}, {0, 0, 1}},
             {{2, {1, 1, 0, 0, 0}, {0}, {0}}}, run);
  auto lozenges = domain_of({{0, 12, 0}, {2, 10, 0}, {4, 12, 0}, {2, 14, 0}, {10, 2, 0}, {12, 0, 0}, {14, 2, 0},
                             {12, 4, 0}}, 3, budget);
  REQUIRE(lozenges.ok());
  check_ball(lozenges.value(), {{2, 10, 0}, {2, 14, 0}}, {{{{2, 10, 0}, {2, 14, 0}}, {{0, 12, 0}, {4, 12, 0}}}, {0, 0, 2, 4, 1}},
             {{3, {4, 4, 0, 1, 1}, {1, 1}, {1, 1}}}, run);
  check_ball(lozenges.value(), {{10, 2, 0}, {14, 2, 0}}, {{{{10, 2, 0}, {14, 2, 0}}, {{12, 0, 0}, {12, 4, 0}}}, {0, 0, 2, 4, 1}},
             {{3, {4, 4, 0, 1, 1}, {1, 1}, {1, 1}}}, run);
  check_ball(lozenges.value(), {{10, 2, 0}, {4, 12, 0}}, {{{{10, 2, 0}, {4, 12, 0}}, {{12, 4, 0}, {2, 10, 0}}}, {0, 0, 2, 4, 1}},
             {{3, {4, 4, 0, 1, 1}, {1, 1}, {1, 1}}}, run);
  check_ball(lozenges.value(), {{10, 2, 0}, {2, 14, 0}}, {{{{10, 2, 0}, {2, 14, 0}}, {{12, 4, 0}, {0, 12, 0}}}, {0, 0, 2, 4, 1}},
             {{3, {20, 4, 4, 11, 2}, {6, 6}, {1, 1}}}, run);
  auto short_line = domain_of({{0, 0, 0}, {2, 0, 0}, {4, 0, 0}}, 2, budget);
  REQUIRE(short_line.ok());
  check_ball(short_line.value(), {{0, 0, 0}, {4, 0, 0}}, {{{{0, 0, 0}, {4, 0, 0}}}, {0, 0, 1}},
             {{2, {3, 2, 2, 1, 1}, {1}, {1}}}, run);
}

// Coquille mixte de l'audit de4ab58a8 : neuf sites de rayon 5, Q_b = 1 paire, 4 triangles, 13 tetraedres ; une
// boule q3 etendue (deux triangles, un interieur) a K3. Puis la meme coquille a l'echelle du profil (homothetie de
// rapport kCoordMax / 10 : budgets de bits des predicats pres du bord) : memes supports et meme fermeture.
MHGP11_TEST(mixed, 196) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto domain = domain_of(mixed(1), 3, budget);
  REQUIRE(domain.ok());
  Run run;
  const Expected big = {{{{5, 5, 0}, {5, 5, 10}},
                         {{5, 2, 1}, {5, 8, 1}, {5, 5, 10}},
                         {{5, 5, 0}, {10, 5, 5}, {2, 5, 9}},
                         {{5, 5, 0}, {2, 5, 9}, {8, 5, 9}},
                         {{2, 1, 5}, {10, 5, 5}, {2, 9, 5}},
                         {{5, 2, 1}, {2, 1, 5}, {2, 9, 5}, {8, 5, 9}},
                         {{5, 2, 1}, {10, 5, 5}, {5, 8, 1}, {2, 5, 9}},
                         {{5, 2, 1}, {10, 5, 5}, {2, 9, 5}, {2, 5, 9}},
                         {{5, 2, 1}, {10, 5, 5}, {2, 9, 5}, {5, 5, 10}},
                         {{5, 2, 1}, {5, 8, 1}, {2, 5, 9}, {8, 5, 9}},
                         {{5, 2, 1}, {2, 9, 5}, {2, 5, 9}, {8, 5, 9}},
                         {{5, 2, 1}, {2, 9, 5}, {5, 5, 10}, {8, 5, 9}},
                         {{5, 5, 0}, {2, 1, 5}, {2, 9, 5}, {8, 5, 9}},
                         {{2, 1, 5}, {10, 5, 5}, {5, 8, 1}, {2, 5, 9}},
                         {{2, 1, 5}, {10, 5, 5}, {5, 8, 1}, {5, 5, 10}},
                         {{2, 1, 5}, {5, 8, 1}, {2, 9, 5}, {8, 5, 9}},
                         {{2, 1, 5}, {5, 8, 1}, {2, 5, 9}, {8, 5, 9}},
                         {{2, 1, 5}, {5, 8, 1}, {5, 5, 10}, {8, 5, 9}}},
                        {0, 0, 1, 11, 54, 94, 79, 36, 9, 1}};
  const std::vector<u32> none(13, 0), one(13, 1);
  auto with = [](std::vector<u32> head, const std::vector<u32>& tail) {
    head.insert(head.end(), tail.begin(), tail.end());
    return head;
  };
  check_ball(domain.value(), {{5, 5, 0}, {5, 5, 10}}, big,
             {{1, {9, 9, 9, 1, 1}, with({1, 0, 0, 0, 0}, none), with({1, 0, 0, 0, 0}, none)},
              {2, {36, 36, 35, 11, 11}, with({7, 1, 1, 1, 1}, none), with({7, 1, 1, 1, 1}, none)},
              {3, {84, 84, 73, 54, 54}, with({21, 6, 6, 6, 6}, one), with({21, 6, 6, 6, 6}, one)}},
             run);
  check_ball(domain.value(), {{5, 2, 1}, {2, 1, 5}, {2, 9, 5}},
             {{{{5, 2, 1}, {2, 1, 5}, {2, 9, 5}}, {{2, 1, 5}, {5, 8, 1}, {2, 9, 5}}}, {0, 0, 0, 2, 1}},
             {{3, {10, 6, 6, 3, 2}, {2, 2}, {1, 1}}}, run);
  const u32 scale = kCoordMax / 10;
  auto large = domain_of(mixed(scale), 1, budget);
  REQUIRE(large.ok());
  std::optional<BallIdx> ball;
  for (u32 b = 0; b < large.value().catalogue().balls(); ++b)
    if (large.value().catalogue().balls_data()[b].m == 9) ball = BallIdx{b};
  REQUIRE(ball.has_value());
  const auto made = supports::ball_supports(large.value(), *ball, run.out, run.scratch);
  REQUIRE(made.ok());
  const auto found = std::span<const Support>(run.out).first(made.value().count);
  CHECK(published_order(found));
  const auto& data = large.value().catalogue().balls_data()[idx(*ball)];
  CHECK(found[0] == (Support{data.support, data.qmin}));
  CHECK(normalized(coordinates(large.value(), found), scale) == normalized(big.supports, 1));
  for (u32 j = 0; j <= 9; ++j) CHECK_EQ(made.value().closure.parts(j), big.closure[j]);
}

// Plafond kMaxShell = 24, fixture d'egalite : 24 sites cocycliques admis (6 diametres, 220 triangles, aucun
// tetraedre : coquille plane), 25 refuses support_shell_capacity par ball_supports comme par ball_shape.
MHGP11_TEST(shell_bound, 16) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  Run run;
  auto admitted = domain_of(cocircular(24), 1, budget);
  REQUIRE(admitted.ok());
  const auto ball = ball_of(admitted.value(), {{27, 14, 7}, {105, 118, 7}});
  REQUIRE(ball.has_value());
  const auto made = supports::ball_supports(admitted.value(), *ball, run.out, run.scratch, &run.ledger);
  REQUIRE(made.ok());
  CHECK_EQ(made.value().count, 226u);
  CHECK_EQ(made.value().closure.shell(), 24u);
  CHECK_EQ(made.value().closure.parts(2), 6u);
  CHECK_EQ(made.value().closure.parts(3), 352u);
  CHECK_EQ(made.value().closure.parts(24), 1u);
  CHECK(published_order(std::span<const Support>(run.out).first(made.value().count)));
  // Tout triangle inscrit strictement aigu contient le centre, coplanaire : 220 orientations, 220 supports.
  CHECK(run.ledger == (supports::SupportLedger{1, 0, 1, 226, 276, 2024, 220, 10626}));
  auto refused = domain_of(cocircular(25), 1, budget);
  REQUIRE(refused.ok());
  const auto wide = ball_of(refused.value(), {{27, 14, 7}, {105, 118, 7}});
  REQUIRE(wide.has_value());
  CHECK_EQ(refused.value().catalogue().balls_data()[idx(*wide)].m, 25u);
  const auto no = supports::ball_supports(refused.value(), *wide, run.out, run.scratch);
  CHECK(!no.ok() && no.outcome().reason == Reason::support_shell_capacity);
  const auto shape = supports::ball_shape(refused.value(), *wide, 1);
  CHECK(!shape.ok() && shape.outcome().reason == Reason::support_shell_capacity);
  CHECK(supports::check_shell(24).ok() && supports::check_shell(25).reason == Reason::support_shell_capacity);
}

// Refus de l'API : BallIdx hors du catalogue, tampons trop courts, domaine des formes, fermeture etrangere.
MHGP11_TEST(refusals, 24) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto domain = domain_of(square(), 2, budget);
  REQUIRE(domain.ok());
  const FullDomain& d = domain.value();
  Run run;
  const BallIdx outside{d.catalogue().balls()};
  auto reason = [](const auto& r) { return r.ok() ? Reason::none : r.outcome().reason; };
  CHECK_EQ(reason(supports::ball_supports(d, outside, run.out, run.scratch)), Reason::parameter_out_of_range);
  CHECK_EQ(reason(supports::ball_shape(d, outside, 2)), Reason::parameter_out_of_range);
  const auto diagonal = ball_of(d, {{0, 0, 0}, {2, 2, 0}});
  const auto side = ball_of(d, {{0, 0, 0}, {2, 0, 0}});
  REQUIRE(diagonal && side);
  const std::span<Support> out(run.out);
  const std::span<u64> scratch(run.scratch);
  CHECK_EQ(reason(supports::ball_supports(d, *diagonal, out.first(1), scratch)), Reason::supports_invariant);
  CHECK_EQ(reason(supports::ball_supports(d, *diagonal, out.first(2), scratch.first(0))), Reason::supports_invariant);
  CHECK_EQ(reason(supports::ball_supports(d, *side, out.first(0), scratch)), Reason::supports_invariant);
  const auto exact = supports::ball_supports(d, *diagonal, out.first(2), scratch.first(1));
  REQUIRE(CHECK(exact.ok()));
  CHECK_EQ(exact.value().count, 2u);
  CHECK_EQ(reason(supports::make_shape(0, 2, 2, 0)), Reason::supports_invariant);
  CHECK_EQ(reason(supports::make_shape(0, 2, 2, 13)), Reason::supports_invariant);
  CHECK_EQ(reason(supports::make_shape(0, 2, 1, 2)), Reason::supports_invariant);
  CHECK_EQ(reason(supports::make_shape(0, 5, 5, 4)), Reason::supports_invariant);
  CHECK_EQ(reason(supports::make_shape(0, 2, 3, 2)), Reason::supports_invariant);
  CHECK_EQ(reason(supports::make_shape(2, 2, 2, 2)), Reason::supports_invariant);
  CHECK_EQ(reason(supports::make_shape(0, 25, 2, 2)), Reason::support_shell_capacity);
  CHECK_EQ(reason(supports::make_shape(0xFFFFFFFFu, 2, 2, 2)), Reason::supports_invariant);  // p + q deborderait
  CHECK(supports::make_shape(11, 24, 2, 12).ok());
  const auto side_shape = supports::ball_shape(d, *side, 2);
  REQUIRE(side_shape.ok());
  CHECK_EQ(reason(supports::ball_counts(side_shape.value(), exact.value().closure)), Reason::supports_invariant);
  CHECK_EQ(reason(supports::ball_counts(side_shape.value(), supports::Closure{})), Reason::supports_invariant);
  const auto regular = supports::ball_supports(d, *side, out, scratch);
  REQUIRE(regular.ok());
  CHECK(supports::ball_counts(side_shape.value(), regular.value().closure).ok());
}

// Constantes et bornes : table de Pascal, brouillons, capacites, comptes extremes du domaine (p = 11, m = 24, K = 12).
MHGP11_TEST(constants, 19) {
  CHECK_EQ(supports::kMaxSupports, 12926u);
  CHECK_EQ(supports::support_capacity(4), 11u);
  CHECK_EQ(supports::closure_words(0), 1u);
  CHECK_EQ(supports::closure_words(6), 1u);
  CHECK_EQ(supports::closure_words(7), 2u);
  CHECK_EQ(supports::closure_words(24), u64{1} << 18);
  CHECK_EQ(supports::closure_words(25), 0u);
  CHECK_EQ(supports::closure_words(0xFFFFFFFFu), 0u);
  const auto extreme = supports::make_shape(11, 24, 2, 12);
  REQUIRE(extreme.ok());
  CHECK_EQ(supports::support_cofaces(extreme.value(), 2), 193536720u);  // C(33, 11)
  CHECK_EQ(supports::support_cofaces(extreme.value(), 4), 20160075u);   // C(31, 9)
  CHECK_EQ(supports::support_gabriel_cofaces(extreme.value(), 2), 1u);  // C(22, 0)
  CHECK_EQ(supports::support_gabriel_cofaces(extreme.value(), 4), 0u);  // C(20, -2)
  const auto low = supports::make_shape(0, 8, 2, 1);
  REQUIRE(low.ok());
  CHECK_EQ(supports::support_cofaces(low.value(), 4), 0u);  // K + 1 < |Q|
  CHECK_EQ(supports::support_cofaces(low.value(), 2), 1u);
  CHECK(low.value().in_window() && low.value().t() == 1u);
  CHECK_EQ(supports::support_cofaces(extreme.value(), 0xFFFFFFFFu), 0u);  // arite hors de 2..4 : aucun support
  CHECK_EQ(supports::support_gabriel_cofaces(extreme.value(), 1), 0u);
}

// Lectures concurrentes du meme domaine, tampons distincts : memes supports et meme fermeture qu'en sequence.
MHGP11_TEST(concurrency, 83) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto domain = domain_of(mixed(1), 3, budget);
  REQUIRE(domain.ok());
  const auto ball = ball_of(domain.value(), {{5, 5, 0}, {5, 5, 10}});
  REQUIRE(ball.has_value());
  Run reference;
  const auto expected = supports::ball_supports(domain.value(), *ball, reference.out, reference.scratch);
  REQUIRE(expected.ok());
  constexpr int kThreads = 4;
  std::vector<Run> runs(kThreads);
  std::vector<u32> counts(kThreads, 0), closures(kThreads, 0);
  std::vector<std::thread> threads;
  for (int t = 0; t < kThreads; ++t)
    threads.emplace_back([&, t] {
      for (int repeat = 0; repeat < 50; ++repeat) {
        const auto made = supports::ball_supports(domain.value(), *ball, runs[t].out, runs[t].scratch);
        if (!made.ok()) return;
        counts[t] = made.value().count;
        closures[t] = made.value().closure.parts(5);
      }
    });
  for (auto& thread : threads) thread.join();
  for (int t = 0; t < kThreads; ++t) {
    CHECK_EQ(counts[t], expected.value().count);
    CHECK_EQ(closures[t], expected.value().closure.parts(5));
    for (u32 i = 0; i < expected.value().count; ++i) CHECK(runs[t].out[i] == reference.out[i]);
  }
}

MHGP11_TEST_MAIN()
