// Boule certifiee et garde entiere (NUM-CERTIFIEE, NUM-GARDE ; CST-0108, CST-0109, CST-0201) : temoins exacts de
// l'auditeur Codex (receipts/audit_contrats_20261007/numerique/witness.py) et du contrat, paragraphe 7.
#include <array>
#include <cstdio>

#include "local_reference.hpp"
#include "num/power_certificate.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace mhgp12::num;
using local_test::acute_corner;
using local_test::point;
using local_test::sphere;

namespace {
std::optional<CertifiedBall> certify(std::initializer_list<Point> support) {
  const std::vector<Point> points(support);
  const auto made = CertifiedBall::certify(points);
  if (!made.ok()) throw std::runtime_error("certificat de test refuse");
  return made.value();
}
CertifiedBall certified(std::initializer_list<Point> support) {
  const auto made = certify(support);
  if (!made) throw std::runtime_error("boule de test non certifiee");
  return *made;
}
Box box(std::array<i64, 3> lo, std::array<i64, 3> hi) {
  const auto made = Box::make(point(lo[0], lo[1], lo[2]), point(hi[0], hi[1], hi[2]));
  if (!made.ok()) throw std::runtime_error("boite de test");
  return made.value();
}
}  // namespace

// NUM-CERTIFIEE : seule une boule dont le centre est prouve dans l'enveloppe convexe de son support recoit la garde.
MHGP12_TEST(certify, 19) {
  CHECK(certify({point(5, 5, 5)}).has_value());
  CHECK(certify({point(0, 0, 0), point(4, 1, 0)}).has_value());
  CHECK(!certify({point(3, 3, 3), point(3, 3, 3)}).has_value());  // degenere
  CHECK(certify({point(0, 0, 0), point(4, 0, 0), point(2, 3, 0)}).has_value());  // aigu
  CHECK(!certify({point(0, 0, 0), point(4, 0, 0), point(0, 4, 0)}).has_value());  // droit : centre sur le bord
  CHECK(!certify({point(0, 0, 0), point(4, 0, 0), point(8, 0, 0)}).has_value());  // aligne
  // Triangle presque aligne : centre a une distance quadratique, bien hors du pave du support.
  CHECK(!certify({point(1000, 1000, 0), point(1100, 1001, 0), point(1200, 1000, 0)}).has_value());
  // Temoin de l'auditeur : S={(419,0,0),(435,15,0),(434,14,0)}, s=5, centre (419/2,479/2,0) ; (0,479,0) est sur la
  // sphere mais hors du pave de l'ancre : la garde le perdrait. La candidate reste dans la voie generique.
  const auto a = point(419, 0, 0), b = point(435, 15, 0), c = point(434, 14, 0);
  CHECK(!certify({a, b, c}).has_value());
  const Sphere candidate = sphere(a, b, c);
  CHECK_EQ(candidate.support_span(), 5);
  REQUIRE(side(candidate, point(0, 479, 0)).ok());
  CHECK_EQ(side(candidate, point(0, 479, 0)).value(), 0);
  CHECK(!(419 - 32 < 0 && 0 < 419 + 64));  // x = 0 sort du pave (419 - 32, 419 + 2*32) qu'aurait cette candidate
  // q4 : centre strictement interieur certifie ; poids nul (centre sur une face) refuse.
  CHECK(certify({point(0, 0, 0), point(2, 2, 0), point(2, 0, 2), point(0, 2, 2)}).has_value());
  CHECK(!certify({point(0, 0, 0), point(4, 0, 0), point(2, 3, 0), point(2, 0, 2)}).has_value());
  CHECK(!certify({point(0, 0, 0), point(2, 0, 0), point(0, 2, 0), point(0, 0, 2)}).has_value());  // centre exterieur
  // Repere du support : coin minimal et etendue.
  const auto ball = certified({point(7, 9, 11), point(10, 9, 11), point(8, 12, 11)});
  CHECK(ball.corner() == (std::array<u32, 3>{7, 9, 11}));
  CHECK_EQ(ball.span(), 2);
  // Hors de 1..4 points : refus avant calcul.
  const std::array<Point, 5> five{};
  CHECK_EQ(CertifiedBall::certify(std::span<const Point>(five)).outcome().reason, Reason::parameter_out_of_range);
  CHECK_EQ(CertifiedBall::certify(std::span<const Point>()).outcome().reason, Reason::parameter_out_of_range);
}

// Sites : contact a la sphere et contact au pave sont deux temoins distincts (la sphere est strictement dans le pave).
// Pave resserre de NUM-GARDE (m - M, m + 2M ; preuve de l'auditeur Codex du 8 octobre, propriete de minimum de la boule
// certifiee) ; les anciens bords (m - 2M, m + 3M) sont graves : meme reponse geometrique, sans arithmetique.
MHGP12_TEST(guard_sites, 43) {
  // q2 (100,100,100)-(104,100,100) : c=(102,100,100), R=2, s=3, M=8, pave ouvert (92,116) sur chaque axe.
  const auto ball = certified({point(100, 100, 100), point(104, 100, 100)});
  const GuardedSphere guard(ball);
  CHECK(guard.lane() == Lane::native);
  GuardLedger ledger;
  const auto sided = [&](i64 x, i64 y, i64 z) {
    const auto got = guard.side(point(x, y, z), &ledger);
    if (!got.ok()) throw std::runtime_error("cote garde refuse");
    const auto generic = side(ball.sphere(), point(x, y, z));
    if (!generic.ok() || generic.value() != got.value()) throw std::runtime_error("garde et voie generique divergent");
    return got.value();
  };
  CHECK_EQ(sided(104, 100, 100), 0);   // sur la sphere
  CHECK_EQ(sided(102, 101, 100), -1);  // dedans
  CHECK_EQ(sided(102, 102, 101), 1);   // juste dehors, dans le pave
  CHECK_EQ(ledger.outside_sites, 0u);
  CHECK_EQ(ledger.lanes.native, 3u);
  CHECK_EQ(sided(115, 100, 100), 1);   // bord interieur du pave, hors de la sphere : arithmetique
  CHECK_EQ(ledger.outside_sites, 0u);
  CHECK_EQ(sided(116, 100, 100), 1);   // juste hors du pave : sans arithmetique
  CHECK_EQ(sided(92, 100, 100), 1);
  CHECK_EQ(sided(100, 100, 93), 1);    // bord interieur bas du pave
  CHECK_EQ(ledger.outside_sites, 2u);
  CHECK_EQ(ledger.lanes.native, 5u);
  CHECK(guard.in_guard({93, 93, 93}) && !guard.in_guard({92, 100, 100}) && !guard.in_guard({100, 116, 100}));
  // Anciens bords interieurs (123 et 85, pave (84,124) d'avant le resserrement) : hors du pave, sans arithmetique.
  CHECK_EQ(sided(123, 100, 100), 1);
  CHECK_EQ(sided(100, 85, 100), 1);
  CHECK_EQ(ledger.outside_sites, 4u);
  CHECK_EQ(ledger.lanes.native, 5u);
  CHECK(!guard.in_guard({123, 100, 100}) && !guard.in_guard({100, 85, 100}));
  // Ecart local hors du domaine des Point, refus au-dela de 2^34.
  CHECK_EQ(guard.side_offset({-100, 0, 0}, &ledger).value(), 1);
  CHECK_EQ(guard.side_offset({i64{1} << 35, 0, 0}).outcome().reason, Reason::parameter_out_of_range);
  // Boule qui deborde son support : q2 (100,100,100)-(107,107,107), s=3, M=8, rayon 7 sqrt(3)/2 ; elle atteint
  // x = 109,56 au-dela de m + M = 108 et x = 97,44 sous m. Ces sites sont dans la boule et dans le pave vrai.
  const auto diagonal = certified({point(100, 100, 100), point(107, 107, 107)});
  const GuardedSphere spread(diagonal);
  for (const auto& p : {std::array<i64, 3>{109, 103, 103}, {98, 103, 103}, {103, 109, 104}, {103, 103, 98}}) {
    const auto got = spread.side(point(p[0], p[1], p[2]));
    REQUIRE(got.ok());
    CHECK_EQ(got.value(), -1);
    CHECK_EQ(got.value(), side(diagonal.sphere(), point(p[0], p[1], p[2])).value());
  }
  // Garde ouverte : la sphere est strictement dans le pave, aucun point de la boule fermee n'est rejete.
  for (i64 x = 84; x <= 124; ++x)
    for (i64 y = 98; y <= 102; ++y)
      if (guard.side(point(x, y, 100)).value() <= 0) CHECK(guard.in_guard({x, y, 100}));
}

// Boites : disjointe du pave rejetee, contact du pave par un coin, boite partielle raffinee et jamais rejetee, boite
// contenue et interieure ; minimum entier contre minimum continu.
MHGP12_TEST(guard_boxes, 60) {
  const auto ball = certified({point(100, 100, 100), point(104, 100, 100)});  // pave (92,116)^3
  const GuardedSphere guard(ball);
  GuardLedger ledger;
  auto signs = guard.bound_signs(box({124, 100, 100}, {130, 101, 101}), &ledger);
  REQUIRE(signs.ok());
  CHECK(signs.value().lower == 1 && signs.value().upper == 1);
  CHECK_EQ(ledger.disjoint_boxes, 1u);
  CHECK_EQ(ledger.lanes.total(), 0u);  // sans arithmetique
  // Contact du pave par un coin : non disjointe, minorant au point (115,115,115), dehors.
  signs = guard.bound_signs(box({115, 115, 115}, {130, 130, 130}), &ledger);
  REQUIRE(signs.ok());
  CHECK(signs.value().lower == 1 && signs.value().upper == 1);
  CHECK_EQ(ledger.disjoint_boxes, 1u);
  CHECK_EQ(ledger.lanes.total(), 1u);
  // Ancien contact par un coin (123,123,123), pave (84,124) d'avant le resserrement : desormais disjointe, sans
  // arithmetique, meme reponse.
  signs = guard.bound_signs(box({123, 123, 123}, {130, 130, 130}), &ledger);
  REQUIRE(signs.ok());
  CHECK(signs.value().lower == 1 && signs.value().upper == 1);
  CHECK_EQ(ledger.disjoint_boxes, 2u);
  CHECK_EQ(ledger.lanes.total(), 1u);
  // Boite partielle qui contient tout le support et le centre (temoin de l'auditeur) : raffinee, jamais rejetee.
  const auto pair = certified({point(100, 100, 100), point(102, 100, 100)});
  const GuardedSphere partial(pair);
  GuardLedger partial_ledger;
  signs = partial.bound_signs(box({0, 0, 0}, {200, 200, 200}), &partial_ledger);
  REQUIRE(signs.ok());
  CHECK(signs.value().lower < 0 && signs.value().upper == 1);
  CHECK_EQ(partial_ledger.partial_boxes, 1u);
  CHECK_EQ(partial_ledger.disjoint_boxes, 0u);
  // Boite contenue dans le pave et dans la boule : coin lointain au budget mixte, interieure.
  const auto wide_pair = certified({point(100, 100, 100), point(108, 100, 100)});  // c=(104,100,100), R=4
  const GuardedSphere inner(wide_pair);
  signs = inner.bound_signs(box({102, 100, 100}, {106, 101, 101}));
  REQUIRE(signs.ok());
  CHECK(signs.value().lower == -1 && signs.value().upper == -1);
  // Minimum entier contre minimum continu (segment de 0 a 1, translate) : F vaut 0 aux deux sites, -1/2 D au milieu.
  const auto unit = certified({point(100, 100, 100), point(101, 100, 100)});
  const GuardedSphere segment(unit);
  signs = segment.bound_signs(box({100, 100, 100}, {101, 100, 100}));
  REQUIRE(signs.ok());
  CHECK_EQ(signs.value().lower, 0);
  CHECK_EQ(signs.value().upper, 0);
  const auto continuous = power_bound_signs(unit.sphere(), box({100, 100, 100}, {101, 100, 100}));
  REQUIRE(continuous.ok());
  CHECK_EQ(continuous.value().lower, -1);
  // Toute decision de la garde sur une boite est celle de la voie generique entiere (LatticeSphere) : minorant entier
  // identique ; boite contenue declaree interieure seulement si elle l'est. Segment unite (centre demi-entier) et
  // triangle aigu a centre fractionnaire (le plancher seul ne donnerait pas le point le plus proche).
  const auto acute = certified({point(100, 100, 100), point(107, 102, 100), point(102, 106, 101)});
  // Petit triangle equilateral : centre (100+2/3, 100+1/3, 100+1/3), R^2 = 2/3. Dans la boite [100,101]^3, le point
  // entier le plus proche (101,100,100) est interieur (distance^2 1/3) quand le plancher (100,100,100) est sur la
  // sphere : seul l'arrondi donne le minimum entier.
  const auto small = certified({point(100, 100, 100), point(101, 101, 100), point(101, 100, 101)});
  const auto cube = GuardedSphere(small).bound_signs(box({100, 100, 100}, {101, 101, 101}));
  REQUIRE(cube.ok());
  CHECK_EQ(cube.value().lower, -1);
  for (const CertifiedBall* ball : {&unit, &acute, &small}) {
    const GuardedSphere g(*ball);
    const LatticeSphere lattice(ball->sphere());
    REQUIRE(lattice.lattice());
    for (i64 x = 88; x <= 114; x += 1)
      for (i64 width : {0, 1, 3}) {
        const Box b = box({x, 99 + width, 100}, {x + width, 101 + 2 * width, 100 + width});
        const auto gs = g.bound_signs(b), ls = lattice.bound_signs(b);
        REQUIRE(gs.ok() && ls.ok());
        CHECK_EQ(gs.value().lower, ls.value().lower);
        if (gs.value().upper < 0) CHECK_EQ(ls.value().upper, -1);
        if (ls.value().upper < 0 && gs.value().lower <= 0) CHECK(gs.value().upper < 0 || gs.value().upper == 1);
      }
  }
}

// Certificats lies a leur domaine (CST-0201) : support aigu d'etendue 20 et requete au coin (3M-1)^3 de l'ANCIEN pave,
// premier produit D|q|^2 >= 2^127 alors que le resultat tient ; le certificat construit pour s l'accepte, celui pour
// s+2 le refuse. Depuis le pave resserre (m - M, m + 2M), ce coin est hors du pave : la garde le rejette sans
// arithmetique (meme reponse) ; le debordement intermediaire est garde par le temoin de l'auditeur, support aigu
// d'etendue 21 (h = 2^21 - 1) et requete (h,h,h) DANS le pave : D|v|^2 = 18 h^6 a 131 bits, puissance finale 2 h^6 a
// 127 bits, essai controle en echec et repli large. La politique des voies (domaine s+2) est inchangee : les mutants qui
// la relachent (domaine s ou s+1) sont des ecarts de politique, vus par le compteur de voies.
MHGP12_TEST(guard_certificate, 40) {
  const i64 m = i64{1} << 20, h = m - 1;
  const auto p = acute_corner(20);
  const auto ball = certified({p[0], p[1], p[2]});
  const Sphere& s = ball.sphere();
  CHECK_EQ(ball.span(), 20);
  const auto d = local_test::widen(s.denominator());
  CHECK(compare(d, local_test::widen(6 * i128{h} * h * h * h)) == 0);
  const std::array<i128, 3> n{4 * i128{h} * h * h * h * h, 2 * i128{h} * h * h * h * h, 2 * i128{h} * h * h * h * h};
  for (int j = 0; j < 3; ++j) CHECK(compare(local_test::widen(s.numerator()[j]), local_test::widen(n[j])) == 0);
  const i128 d128 = 6 * i128{h} * h * h * h;
  CHECK(mhgp12::num::detail::power_certificate_i128(d128, n, 20));
  CHECK(!mhgp12::num::detail::power_certificate_i128(d128, n, 22));
  CHECK(s.power_domain() >= 20 && s.power_domain() < 22);
  const GuardedSphere guard(ball);
  CHECK(guard.lane() == Lane::checked);
  // Requete au coin de l'ancien pave : q = (3M-1)^3, hors du pave resserre (-M, 2M) ; faits arithmetiques conserves.
  const std::array<i64, 3> q{3 * m - 1, 3 * m - 1, 3 * m - 1};
  CHECK(!guard.in_guard(q));
  const auto first = local_test::product(d, local_test::widen(i128{3} * q[0] * q[0]));
  CHECK_EQ(first.bit_length(), 128);  // 215333976975021689807285368499032031250 >= 2^127
  const auto exact = local_test::power(s, q);
  const i128 expected = static_cast<i128>((u128{8214531360641908690ull} << 64) | 9223704089311838210ull);
  CHECK(compare(exact, local_test::widen(expected)) == 0);  // 151531357695222388613514543567625781250 < 2^127
  GuardLedger ledger;
  const auto got = guard.side_offset(q, &ledger);
  REQUIRE(got.ok());
  CHECK_EQ(got.value(), 1);  // meme reponse geometrique, sans arithmetique
  CHECK(ledger.lanes == (LaneCount{0, 0, 0, 0}));
  CHECK_EQ(ledger.outside_sites, 1u);
  // Temoin de l'auditeur (receipts/audit_reponses_20261008/garde_census) : etendue 21, requete (h,h,h) dans le pave.
  {
    const i64 h21 = (i64{1} << 21) - 1;
    const auto p21 = acute_corner(21);
    const auto ball21 = certified({p21[0], p21[1], p21[2]});
    const Sphere& s21 = ball21.sphere();
    CHECK_EQ(ball21.span(), 21);
    CHECK_EQ(s21.power_domain(), 17);
    const auto d21 = local_test::widen(s21.denominator());
    CHECK(compare(d21, local_test::widen(6 * i128{h21} * h21 * h21 * h21)) == 0);
    const GuardedSphere g21(ball21);
    CHECK(g21.lane() == Lane::checked);
    const std::array<i64, 3> v{h21, h21, h21};
    CHECK(g21.in_guard(v));
    CHECK_EQ(local_test::product(d21, local_test::widen(i128{3} * h21 * h21)).bit_length(), 131);  // 18 h^6
    const auto power21 = local_test::power(s21, v);  // 2 h^6
    CHECK_EQ(power21.bit_length(), 127);
    CHECK_EQ(power21.sign(), 1);
    GuardLedger wide;
    const auto side21 = g21.side_offset(v, &wide);
    REQUIRE(side21.ok());
    CHECK_EQ(side21.value(), 1);
    CHECK(wide.lanes == (LaneCount{0, 0, 0, 1}));  // essai controle en echec au premier produit, repli large
  }
  // Dans le pave, mais pres du support : l'essai controle tient.
  GuardLedger near;
  CHECK_EQ(guard.side_offset({h, h, h}, &near).value(), local_test::power(s, {h, h, h}).sign());
  CHECK(near.lanes == (LaneCount{0, 0, 1, 0}));
  // Certificat au domaine s+1 mais pas s+2 (support aigu d'etendue 20 trouve par recherche) : la voie gardee reste
  // controlee ; une garde d'un bit trop etroite la ferait certifier.
  const auto other = certified({point(0, 0, 0), point(821959, 633415, 78800), point(178081, 356898, 963792)});
  CHECK_EQ(other.span(), 20);
  CHECK_EQ(other.sphere().power_domain(), 21);
  CHECK(GuardedSphere(other).lane() == Lane::checked);
  // Paliers de la garde : s* = 16 natif ; s = 17 et 19 certifies (le pire support y tient au domaine s+2). Requete au
  // coin du pave resserre (2M - 1 par axe).
  for (const int span : {16, 17, 19}) {
    const auto corner = acute_corner(span);
    const auto worst = certified({corner[0], corner[1], corner[2]});
    const GuardedSphere g(worst);
    CHECK(g.lane() == (span == 16 ? Lane::native : Lane::certified));
    const i64 edge = 2 * (i64{1} << span) - 1;
    CHECK(g.in_guard({edge, edge, edge}));
    GuardLedger l;
    CHECK_EQ(g.side_offset({edge, edge, edge}, &l).value(), local_test::power(worst.sphere(), {edge, edge, edge}).sign());
    CHECK(l.lanes == (span == 16 ? LaneCount{1, 0, 0, 0} : LaneCount{0, 1, 0, 0}));
  }
}

MHGP12_TEST_MAIN()
