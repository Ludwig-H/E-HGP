// Temoins exacts des auditeurs pour les PRIMITIVES du module supports (ball_supports, ball_shape, make_shape,
// ball_counts, support_cofaces), sans arbre FULL ni assemblage (tranche S6b). Sources : note mathematique des
// auditeurs (audits/AUDIT_REPONSES_AUX_VERROUS_MOTEUR_20261002.md), recus
// receipts/audit_supports_contract_20261005/qb et receipts/audit_native_integration_20261005/qb (check_high_k.py),
// puis receipts/audit_geant_20261005/native (P2), 5 octobre 2026. Attendus graves depuis ces recus (enumerations et
// preuves des auditeurs, independantes du produit), jamais lus dans le produit.
//   sphere5        les 24 points entiers de x^2 + y^2 + z^2 = 5, translates de (2, 2, 2) : coquille mixte au plafond
//                  kMaxShell = 24, admise ; Q_b = 12 diametres, 24 triangles, 792 tetraedres ; N_2 = 12, N_3 = 288,
//                  N_4 = 3906 ; comptes de K1 a K3 ; somme des cofaces par support 4 068 a K3 ; les q4 restent dans
//                  Q_b a K1 malgre zero coface.
//   sphere5_k12    fermeture de la boule centrale de Cat_1 par ball_supports, puis make_shape(0, 24, 2, 12) et
//                  ball_counts a K12 : 2 704 156 K-parties reliees et parties comprimees, 116 traces strictes,
//                  2 496 144 cofaces distinctes et de Gabriel, 149 954 688 incidences supports -> cofaces (qui ne
//                  remplacent pas les cofaces distinctes). Ni foret FULL12, ni ball_shape a K12 sur un domaine Cat_1.
//   square_k10     quatre coins d'un carre de cote 20 et huit sites interieurs (receipts/.../qb/normal.json) : boule
//                  de centre (10, 10, 10), niveau 200, p = 8, m = 4, qmin = 2 ; N = (0, 0, 2, 4, 1) ; a K10 : 66, 6, 4,
//                  12 et 4 ; deux diametres de 10 incidences chacun.
//   sphere9_refus  25 des 30 points entiers de x^2 + y^2 + z^2 = 9, les six points axiaux gardes, translates de
//                  (3, 3, 3) : coquille de 25 sites, refus support_shell_capacity de la primitive (ball_supports,
//                  ball_shape) ; le refus de l'appel supports entier viendra avec l'assemblage (S6b).
// Ces boules ont une presentation canonique q2 : ces portes ne remplacent pas les portes numeriques des fabriques
// initiales q3 et q4 du catalogue.
//   impossible_arity  audit general a65903a7b, P2 : support_cofaces et support_gabriel_cofaces rendent 0 pour une
//                  arite superieure a m (formes (1, 2, 2, 2), (2, 2, 2, 3) et (1, 3, 3, 3), arites 3 et 4) ; balayage
//                  du domaine de Shape : 4 401 formes, 300 couples (forme, arite > m), dont 51 que l'ancienne garde
//                  (2 <= a <= 4 seulement) comptait non nuls ; mutant arite_impossible_admise.
#include <optional>
#include <vector>

#include "supports_support.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace supports_test;
using supports::BallCounts;
using supports::Support;

namespace {

struct Buffers {
  std::vector<Support> out = std::vector<Support>(supports::kMaxSupports);
  std::vector<u64> scratch = std::vector<u64>(supports::kMaxClosureWords);
};

// Points entiers de x^2 + y^2 + z^2 = r2 (|coordonnee| <= 3), ordre lexicographique, translates de (t, t, t).
Points sphere(i32 r2, u32 t) {
  Points out;
  for (i32 x = -3; x <= 3; ++x)
    for (i32 y = -3; y <= 3; ++y)
      for (i32 z = -3; z <= 3; ++z)
        if (x * x + y * y + z * z == r2) out.push_back({x + t, y + t, z + t});
  return out;
}

// La seule boule du catalogue a coquille de m sites ; nullopt s'il n'y en a pas exactement une.
std::optional<BallIdx> ball_with_shell(const FullDomain& domain, u32 m) {
  std::optional<BallIdx> found;
  const auto balls = domain.catalogue().balls_data();
  for (u32 b = 0; b < balls.size(); ++b) {
    if (balls[b].m != m) continue;
    if (found) return std::nullopt;
    found = BallIdx{b};
  }
  return found;
}

u64 binomial(u32 n, u32 k) {
  if (k > n) return 0;
  u64 c = 1;
  for (u32 i = 1; i <= k; ++i) c = c * (n - k + i) / i;
  return c;
}

}  // namespace

// Sphere5 a K1, K2, K3 (domaine Cat_3) : Q_b et sa fermeture, puis comptes par boule et par support.
MHGP11_TEST(sphere5, 35) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const Points points = sphere(5, 2);
  REQUIRE(CHECK_EQ(points.size(), 24u));
  auto domain = domain_of(points, 3, budget);
  REQUIRE(domain.ok());
  const auto ball = ball_with_shell(domain.value(), 24);
  REQUIRE(ball.has_value());
  const auto& data = domain.value().catalogue().balls_data()[idx(*ball)];
  CHECK(data.p == 0u && data.qmin == 2u);
  Buffers run;
  const auto made = supports::ball_supports(domain.value(), *ball, run.out, run.scratch);
  REQUIRE(made.ok());
  const auto found = std::span<const Support>(run.out).first(made.value().count);
  REQUIRE(CHECK_EQ(found.size(), 828u));
  CHECK(published_order(found));
  CHECK(found[0] == (Support{data.support, data.qmin}));
  std::array<u32, 5> by_arity{};
  for (const Support& s : found) ++by_arity[s.arity];
  CHECK_EQ(by_arity[2], 12u);
  CHECK_EQ(by_arity[3], 24u);
  CHECK_EQ(by_arity[4], 792u);
  const auto& closure = made.value().closure;
  CHECK_EQ(closure.shell(), 24u);
  CHECK_EQ(closure.parts(1), 0u);
  CHECK_EQ(closure.parts(2), 12u);
  CHECK_EQ(closure.parts(3), 288u);
  CHECK_EQ(closure.parts(4), 3906u);
  CHECK_EQ(closure.parts(24), 1u);
  // (K, kparties_reliees, strict_traces, cofaces) du recu ; p = 0 : parties comprimees = K-parties reliees, cofaces de
  // Gabriel = cofaces.
  const std::array<std::array<u32, 4>, 3> table = {{{1, 24, 24, 12}, {2, 276, 264, 288}, {3, 2024, 1736, 3906}}};
  for (const auto& row : table) {
    const auto shape = supports::ball_shape(domain.value(), *ball, static_cast<Order>(row[0]));
    REQUIRE(shape.ok());
    const auto counts = supports::ball_counts(shape.value(), closure);
    REQUIRE(counts.ok());
    CHECK(counts.value() == (BallCounts{row[1], row[1], row[2], row[3], row[3]}));
    u64 incidences = 0, gabriel = 0, q4 = 0;
    for (const Support& s : found) {
      incidences += supports::support_cofaces(shape.value(), s.arity);
      gabriel += supports::support_gabriel_cofaces(shape.value(), s.arity);
      if (s.arity == 4) q4 += supports::support_cofaces(shape.value(), s.arity);
    }
    CHECK_EQ(gabriel, incidences);
    if (row[0] == 1) {
      // Les 792 tetraedres sont publies a K1 avec zero coface (K + 1 < 4) ; seules les diametres en portent une.
      CHECK_EQ(q4, 0u);
      CHECK_EQ(incidences, 12u);
    } else if (row[0] == 2) {
      CHECK_EQ(incidences, 288u);
    } else {
      CHECK_EQ(incidences, 4068u);  // incidences (Q, G), distinctes des 3 906 cofaces
    }
  }
}

// Sphere5 a K12 par les primitives : fermeture de Cat_1, forme (0, 24, 2, 12), comptes et incidences.
MHGP11_TEST(sphere5_k12, 28) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto domain = domain_of(sphere(5, 2), 1, budget);
  REQUIRE(domain.ok());
  const auto ball = ball_with_shell(domain.value(), 24);
  REQUIRE(ball.has_value());
  Buffers run;
  const auto made = supports::ball_supports(domain.value(), *ball, run.out, run.scratch);
  REQUIRE(made.ok());
  REQUIRE(CHECK_EQ(made.value().count, 828u));
  const auto& closure = made.value().closure;
  // Toute partie de 13 sites ou plus contient une paire antipodale ; a 12 sites, 116 des 4 096 choix sans paire sont
  // separables (recu check_high_k.py, deux calculs independants).
  CHECK_EQ(closure.parts(12), 2704040u);
  for (u32 j = 13; j <= 24; ++j) CHECK_EQ(closure.parts(j), binomial(24, j));
  const auto shape = supports::make_shape(0, 24, 2, 12);
  REQUIRE(shape.ok());
  const auto counts = supports::ball_counts(shape.value(), closure);
  REQUIRE(counts.ok());
  CHECK(counts.value() == (BallCounts{2704156, 2704156, 116, 2496144, 2496144}));
  std::array<u64, 5> per_arity{}, gabriel_per_arity{};
  u64 incidences = 0;
  for (u32 i = 0; i < made.value().count; ++i) {
    const u32 a = run.out[i].arity;
    per_arity[a] = supports::support_cofaces(shape.value(), a);
    gabriel_per_arity[a] = supports::support_gabriel_cofaces(shape.value(), a);
    incidences += supports::support_cofaces(shape.value(), a);
  }
  CHECK_EQ(per_arity[2], 705432u);  // C(22, 11)
  CHECK_EQ(per_arity[3], 352716u);  // C(21, 10)
  CHECK_EQ(per_arity[4], 167960u);  // C(20, 9)
  CHECK(gabriel_per_arity == per_arity);  // p = 0 : toute coface contient I_b
  CHECK_EQ(incidences, u64{149954688});
  CHECK(incidences > counts.value().cofaces);  // les incidences ne sont pas des cofaces distinctes
  CHECK_EQ(counts.value().kparties_reliees, binomial(24, 12));
}

// Petit temoin a K10 (12 sites, domaine Cat_10) : la boule du carre a huit sites interieurs.
MHGP11_TEST(square_k10, 24) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const Points corners = {{0, 0, 10}, {0, 20, 10}, {20, 0, 10}, {20, 20, 10}};
  Points points = corners;
  for (const Xyz& p : Points{{9, 9, 10}, {9, 10, 10}, {9, 11, 10}, {10, 9, 10}, {10, 10, 10}, {10, 11, 10},
                             {11, 9, 10}, {11, 10, 10}})
    points.push_back(p);
  auto domain = domain_of(points, 10, budget);
  REQUIRE(domain.ok());
  std::optional<BallIdx> ball;
  u32 matches = 0;
  const auto balls = domain.value().catalogue().balls_data();
  for (u32 b = 0; b < balls.size(); ++b)
    if (balls[b].p == 8 && balls[b].m == 4) {
      ball = BallIdx{b};
      ++matches;
    }
  REQUIRE(CHECK_EQ(matches, 1u));
  CHECK_EQ(balls[idx(*ball)].qmin, 2u);
  Buffers run;
  const auto made = supports::ball_supports(domain.value(), *ball, run.out, run.scratch);
  REQUIRE(made.ok());
  const auto found = std::span<const Support>(run.out).first(made.value().count);
  REQUIRE(CHECK_EQ(found.size(), 2u));
  CHECK(published_order(found));
  const std::vector<Points> diagonals = {{{0, 0, 10}, {20, 20, 10}}, {{0, 20, 10}, {20, 0, 10}}};
  CHECK(normalized(coordinates(domain.value(), found), 1) == normalized(diagonals, 1));
  const auto& closure = made.value().closure;
  REQUIRE(CHECK_EQ(closure.shell(), 4u));
  const std::array<u32, 5> want = {0, 0, 2, 4, 1};
  for (u32 j = 0; j <= 4; ++j) CHECK_EQ(closure.parts(j), want[j]);
  const auto shape = supports::ball_shape(domain.value(), *ball, 10);
  REQUIRE(shape.ok());
  const auto counts = supports::ball_counts(shape.value(), closure);
  REQUIRE(counts.ok());
  CHECK(counts.value() == (BallCounts{66, 6, 4, 12, 4}));
  u64 incidences = 0;
  for (const Support& s : found) {
    CHECK_EQ(supports::support_cofaces(shape.value(), s.arity), 10u);         // C(10, 9)
    CHECK_EQ(supports::support_gabriel_cofaces(shape.value(), s.arity), 2u);  // C(2, 1)
    incidences += supports::support_cofaces(shape.value(), s.arity);
  }
  CHECK_EQ(incidences, 20u);
}

// Refus au plafond : coquille de 25 sites (sphere9 privee de 5 sites non axiaux), refusee par la primitive.
MHGP11_TEST(sphere9_refus, 12) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const Points all = sphere(9, 3);
  REQUIRE(CHECK_EQ(all.size(), 30u));
  Points kept;
  u32 dropped = 0;
  for (auto it = all.rbegin(); it != all.rend(); ++it) {  // les 5 derniers sites non axiaux sont retires
    const Xyz& p = *it;
    const u32 axial = (p[0] == 3 ? 1u : 0u) + (p[1] == 3 ? 1u : 0u) + (p[2] == 3 ? 1u : 0u);
    if (axial != 2 && dropped < 5) {
      ++dropped;
      continue;
    }
    kept.push_back(p);
  }
  REQUIRE(CHECK_EQ(kept.size(), 25u));
  auto domain = domain_of(kept, 1, budget);
  REQUIRE(domain.ok());
  const auto ball = ball_with_shell(domain.value(), 25);
  REQUIRE(ball.has_value());
  const auto& data = domain.value().catalogue().balls_data()[idx(*ball)];
  CHECK(data.p == 0u && data.qmin == 2u);
  Buffers run;
  auto reason = [](const auto& r) { return r.ok() ? Reason::none : r.outcome().reason; };
  CHECK_EQ(reason(supports::ball_supports(domain.value(), *ball, run.out, run.scratch)),
           Reason::support_shell_capacity);
  CHECK_EQ(reason(supports::ball_shape(domain.value(), *ball, 1)), Reason::support_shell_capacity);
  CHECK_EQ(supports::check_shell(25).reason, Reason::support_shell_capacity);
  CHECK(supports::check_shell(24).ok());
  CHECK_EQ(reason(supports::make_shape(0, 25, 2, 1)), Reason::support_shell_capacity);
}

// Arite impossible (a > m) : aucun support de cette arite sur la coquille, donc aucune incidence (P2 de a65903a7b).
MHGP11_TEST(impossible_arity, 25) {
  struct Row {
    u32 p, m, q;
    Order k;
    std::array<u32, 3> cofaces, gabriel;  // arites 2, 3, 4
  };
  const std::array<Row, 3> rows = {{{1, 2, 2, 2, {1, 0, 0}, {1, 0, 0}},
                                    {2, 2, 2, 3, {1, 0, 0}, {1, 0, 0}},
                                    {1, 3, 3, 3, {1, 1, 0}, {1, 1, 0}}}};
  for (const Row& row : rows) {
    const auto shape = supports::make_shape(row.p, row.m, row.q, row.k);
    REQUIRE(shape.ok());
    for (u32 a = 2; a <= 4; ++a) {
      CHECK_EQ(supports::support_cofaces(shape.value(), a), row.cofaces[a - 2]);
      CHECK_EQ(supports::support_gabriel_cofaces(shape.value(), a), row.gabriel[a - 2]);
    }
  }
  // Balayage du domaine de Shape (1 <= K <= 12, 2 <= qmin <= 4, qmin <= m <= 24, p + qmin <= K + 1).
  u32 shapes = 0, impossible = 0, formerly = 0, nonzero = 0;
  for (Order k = 1; k <= supports::kMaxOrder; ++k)
    for (u32 q = 2; q <= 4; ++q)
      for (u32 m = q; m <= supports::kMaxShell; ++m)
        for (u32 p = 0; p + q <= u32{k} + 1; ++p) {
          const auto shape = supports::make_shape(p, m, q, k);
          if (!shape.ok()) continue;
          ++shapes;
          for (u32 a = m + 1; a <= 4; ++a) {
            ++impossible;
            const i32 x = static_cast<i32>(p + m) - static_cast<i32>(a), y = i32{k} + 1 - static_cast<i32>(a);
            formerly += supports::supports_detail::binomial(x, y) != 0 ? 1 : 0;
            const bool counted = supports::support_cofaces(shape.value(), a) != 0 ||
                                 supports::support_gabriel_cofaces(shape.value(), a) != 0;
            nonzero += counted ? 1 : 0;
          }
        }
  CHECK_EQ(shapes, 4401u);
  CHECK_EQ(impossible, 300u);
  CHECK_EQ(formerly, 51u);
  CHECK_EQ(nonzero, 0u);
}
