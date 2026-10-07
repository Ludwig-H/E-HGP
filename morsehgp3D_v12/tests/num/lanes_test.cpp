// Portes de palier (docs/CONTRAT_NUMERIQUE.md, paragraphe 7) : pour chaque expression implantee, un temoin a
// l'etendue limite s* et un a s*+1 ; le compteur de voies prouve que le repli est effectivement emprunte, et la valeur
// est jugee contre une reference toujours large (local_reference.hpp). Les temoins au-dela du profil compile (etendue
// > B) sont joues au profil qui les admet ; leurs controles sont comptes a part (affichage `lanes_*`).
#include <array>
#include <cstdio>

#include "local_reference.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace mhgp12::num;
using local_test::acute_corner;
using local_test::point;
using local_test::sphere;

namespace {
LaneCount only(Lane lane) {
  LaneCount out;
  out.add(lane);
  return out;
}
i64 top(int s) { return (i64{1} << s) - 1; }
}  // namespace

// Cote d'un point du repere, 6s+8 : natif au palier etroit (s* = 16), certifie au-dela (s = 17), repli controle ou
// large quand le certificat ne couvre plus la requete (s = 20, requete d'etendue 21 ; temoin "cote q3 a s = 19 et 20").
MHGP12_TEST(lane_side, 33) {
  for (const int s : {16, 17, 19, 20}) {
    const auto p = acute_corner(s);
    const Sphere ball = sphere(p[0], p[1], p[2]);
    CHECK_EQ(ball.support_span(), s);
    const auto query = point(top(s), top(s), top(s));
    LaneCount lanes, power_lanes;
    const auto got = side(ball, query, &lanes);
    const auto value = power(ball, query, &power_lanes);
    REQUIRE(got.ok() && value.ok());
    const auto expected = local_test::power(ball, local_test::offset(ball, {top(s), top(s), top(s)}));
    CHECK_EQ(got.value(), expected.sign());
    CHECK(compare(local_test::widen(value.value()), expected) == 0);
    CHECK(lanes == only(s <= kNarrowSpan ? Lane::native : Lane::certified));
    CHECK(power_lanes == lanes);
  }
  // s = 20 (D = 6 h^4 > 2^81) contre une requete d'etendue 21 : le certificat de domaine 21 tombe, repli.
  const auto p = acute_corner(20);
  const Sphere ball = sphere(p[0], p[1], p[2]);
  CHECK_EQ(ball.power_domain(), 20);
  const auto far = point(top(21), top(21), top(21));
  LaneCount lanes;
  const auto got = side(ball, far, &lanes);
  REQUIRE(got.ok());
  CHECK_EQ(got.value(), local_test::power(ball, local_test::offset(ball, {top(21), top(21), top(21)})).sign());
  CHECK_EQ(lanes.native + lanes.certified, 0u);
  CHECK_EQ(lanes.checked + lanes.wide, 1u);
  // Bornes de boite : meme voie que le cote pour la meme etendue.
  LaneCount bound_lanes;
  const auto box = Box::make(point(0, 0, 0), far);
  REQUIRE(box.ok());
  REQUIRE(power_bounds(ball, box.value(), &bound_lanes).ok());
  CHECK_EQ(bound_lanes.native + bound_lanes.certified, 0u);
  CHECK_EQ(bound_lanes.checked + bound_lanes.wide, 1u);
}

// Orientation avec centre, 7s+9 : natif au palier etroit (s* = 16), certifie a s = 17 (le pire support y tient encore),
// large a s = 18 (D = 6 h^4 > 2^70).
MHGP12_TEST(lane_orientation, 12) {
  for (const int s : {16, 17, 18}) {
    const auto p = acute_corner(s);
    const Sphere ball = sphere(p[0], p[1], p[2]);
    const auto a = point(0, 0, 0), b = point(top(s), 0, 0), c = point(0, top(s), 0);
    LaneCount lanes;
    const auto got = orientation(a, b, c, ball, &lanes);
    REQUIRE(got.ok());
    CHECK_EQ(got.value(), local_test::orientation(a, b, c, ball));
    CHECK(got.value() != 0);
    CHECK(lanes == only(s == 16 ? Lane::native : s == 17 ? Lane::certified : Lane::wide));
  }
}

// Test du milieu en forme locale, 5s+6 : natif jusqu'au palier moyen (s* = 24), large au-dela (profil 32).
MHGP12_TEST(lane_midpoint, 4) {
  for (const int s : {21, 24, 25}) {
    if (s > kCoordBits) continue;
    const auto a = point(0, 0, 0), b = point(top(s), top(s) - 1, 1);
    const Sphere ball = sphere(a, b);
    LaneCount lanes;
    CHECK(is_midpoint(ball, a, b, &lanes));
    CHECK(!is_midpoint(ball, a, point(top(s), top(s), 1), &lanes));
    CHECK(lanes.native + lanes.wide == 2u);
    CHECK_EQ(s <= kMediumSpan ? lanes.native : lanes.wide, 2u);
    std::printf("lanes_midpoint s=%d native=%llu wide=%llu\n", s, static_cast<unsigned long long>(lanes.native),
                static_cast<unsigned long long>(lanes.wide));
  }
}

// Distance carree des requetes a centre entier (NUM-REQUETE) : native jusqu'a l'etendue 31, controlee puis u128 a 32.
MHGP12_TEST(lane_distance, 4) {
  const i64 m = kCoordMax;
  LaneCount lanes;
  CHECK(squared_distance(point(0, 0, 0), point(m, m, m), &lanes) == 3 * static_cast<DotInt>(m) * m);
  CHECK(lanes.total() == 1u);
  CHECK_EQ(kCoordBits <= 31 ? lanes.native : lanes.wide, 1u);
  if constexpr (kCoordBits > 31) {
    LaneCount single;
    CHECK(squared_distance(point(0, 0, 0), point(m, 0, 0), &single) == static_cast<DotInt>(m) * m);
    CHECK(single == only(Lane::checked));
    LaneCount narrow;
    CHECK(squared_distance(point(0, 0, 0), point(top(31), top(31), top(31)), &narrow) ==
          3 * static_cast<DotInt>(top(31)) * top(31));
    CHECK(narrow == only(Lane::native));
  } else {
    CHECK(lanes.native == 1u);
  }
}

// Distance du reservoir, 2s+4 : i64 aux paliers etroit et moyen (s* = 24), i128 au palier large ; temoin de
// l'auditeur (CST-0208) : s = 30, boite [0,1]^3, site (2^30-1)^3, valeur 3(2^31-3)^2 au-dela de i64.
MHGP12_TEST(lane_reservoir, 20) {
  for (const int s : {24, 25, 30}) {
    Frame frame;
    REQUIRE(frame.add_box({0, 0, 0}, {1, 1, 1}).ok());
    const u32 x = static_cast<u32>(top(s));
    frame.add_site(x, x, x);
    CHECK_EQ(frame.span(), s);
    LaneCount lanes;
    const auto value = reservoir_distance(frame, {x, x, x}, {0, 0, 0}, {1, 1, 1}, &lanes);
    REQUIRE(value.ok());
    const u128 twice = 2 * u128{x} - 1;
    CHECK(value.value() == 3 * twice * twice);
    CHECK(lanes == only(s <= kMediumSpan ? Lane::native : Lane::wide));
    if (s == 30) CHECK(value.value() >= (u128{1} << 63));
  }
  Frame frame;
  REQUIRE(frame.add_box({0, 0, 0}, {1, 1, 1}).ok());
  CHECK_EQ(reservoir_distance(frame, {2, 0, 0}, {0, 0, 0}, {1, 1, 1}).outcome().reason, Reason::parameter_out_of_range);
  CHECK_EQ(reservoir_distance(frame, {1, 1, 1}, {1, 0, 0}, {0, 1, 1}).outcome().reason, Reason::parameter_out_of_range);
  CHECK_EQ(reservoir_distance(Frame(), {0, 0, 0}, {0, 0, 0}, {0, 0, 0}).outcome().reason,
           Reason::parameter_out_of_range);
}

// Numerateur du centre q3, 5s+5, et q4, 4s+5 : i128 jusqu'au palier moyen (s* = 24), entiers exacts au-dela.
MHGP12_TEST(lane_center, 4) {
  for (const int s : {21, 24, 25}) {
    if (s > kCoordBits) continue;
    const auto p = acute_corner(s);
    LaneCount lanes;
    const auto made = Q3Candidate::through(p[0], p[1], p[2], &lanes);
    REQUIRE(made.ok() && made.value());
    CHECK(lanes == only(s <= kMediumSpan ? Lane::native : Lane::wide));
    const auto q4 = Q4Candidate::through(p[0], p[1], p[2], point(0, top(s), top(s)), &lanes);
    REQUIRE(q4.ok() && q4.value());
    CHECK_EQ(s <= kMediumSpan ? lanes.native : lanes.wide, 2u);
  }
}

// Comparaison de niveaux, 14s+20 : i128 quand les deux produits croises tiennent en 127 bits, sinon largeur du palier.
MHGP12_TEST(lane_levels, 9) {
  const auto level = [](u128 numerator, u128 denominator) {
    auto made = Level::make(to_wide(numerator), to_wide(denominator));
    if (!made.ok()) throw std::runtime_error("niveau de test");
    return made.value();
  };
  const u128 edge = u128{1} << 125;  // 126 bits : produit par un denominateur de 1 bit, 127 bits
  LaneCount native, wide;
  CHECK_EQ(compare(level(edge, 1), level(edge + 1, 1), &native), -1);
  CHECK_EQ(compare(level(edge + 1, 1), level(edge, 1), &native), 1);
  CHECK(native == (LaneCount{2, 0, 0, 0}));
  CHECK_EQ(compare(level(2 * edge, 1), level(2 * edge + 1, 1), &wide), -1);  // 127 + 1 bits : repli
  CHECK_EQ(compare(level(2 * edge + 1, 1), level(2 * edge, 1), &wide), 1);
  CHECK_EQ(compare(level(2 * edge, 3), level(2 * edge, 3), &wide), 0);
  CHECK(wide == (LaneCount{0, 0, 0, 3}));
  // Niveaux reels : q3 d'etendue 7 (comparaison native) contre q4 du domaine (repli).
  const auto p = acute_corner(7);
  const auto small = sphere(p[0], p[1], p[2]);
  LaneCount mixed;
  CHECK_EQ(compare(small.level(), small.level(), &mixed), 0);
  CHECK(mixed == (LaneCount{1, 0, 0, 0}));
}

// Ordre des centres en deux temps, parties fractionnaires 8s+10 : natives si les longueurs le permettent (s = 15),
// largeur du palier au-dela (s = 17). Deux presentations du meme centre : chaque axe compare ses fractions.
MHGP12_TEST(lane_centers_order, 6) {
  for (const int s : {15, 17}) {
    const auto p = acute_corner(s);
    const Sphere first = sphere(p[0], p[1], p[2]), second = sphere(p[1], p[2], p[0]);
    LaneCount lanes;
    CHECK_EQ(compare_centers(first, second, &lanes), 0);
    CHECK_EQ(compare_centers(second, first, &lanes), 0);
    CHECK(lanes == (s == 15 ? LaneCount{6, 0, 0, 0} : LaneCount{0, 0, 0, 6}));
    std::printf("lanes_centers s=%d native=%llu wide=%llu\n", s, static_cast<unsigned long long>(lanes.native),
                static_cast<unsigned long long>(lanes.wide));
  }
}
