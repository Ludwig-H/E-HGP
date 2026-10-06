// Bornes de census sur sites entiers (LatticeSphere, levier V3) : minorant = minimum exact de la puissance sur les
// points entiers de la boite, enumeres ; majorant = maximum exact aux huit coins. Reference : puissance large du
// harnais (power_reference.hpp), sans selection native. La borne continue power_bound_signs reste la reference de
// monotonie : chaque decision qu'elle prend, la borne entiere la prend aussi.
#include <array>
#include <stdexcept>

#include "num/num.hpp"
#include "power_reference.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::num;

namespace {
Point point(i64 x, i64 y, i64 z) {
  auto made = Point::make(x, y, z);
  if (!made.ok()) throw std::runtime_error("fixture lattice hors domaine");
  return made.value();
}

Box box(Point lo, Point hi) {
  auto made = Box::make(lo, hi);
  if (!made.ok()) throw std::runtime_error("fixture lattice boite inversee");
  return made.value();
}

std::optional<Sphere> through(u8 q, const std::array<Point, 4>& p) {
  if (q == 1) return Sphere::point(p[0]);
  auto made = q == 2 ? Sphere::through(p[0], p[1]) :
              q == 3 ? Sphere::through(p[0], p[1], p[2]) : Sphere::through(p[0], p[1], p[2], p[3]);
  if (!made.ok()) throw std::runtime_error("fixture lattice refus de sphere");
  return made.value();
}

Sphere sphere(u8 q, const std::array<Point, 4>& p) {
  auto made = through(q, p);
  if (!made) throw std::runtime_error("fixture lattice support degenere");
  return *made;
}

int sign(const Wide<4>& value) { return value.sign(); }

struct Truth { int lower, upper; };

// Minimum sur la boite inter Z^3 par enumeration, maximum aux huit coins (puissance convexe).
Truth truth(const Sphere& s, const Box& b) {
  const auto lo = b.lo().coordinates(), hi = b.hi().coordinates();
  bool first = true;
  Wide<4> least, most;
  for (i64 x = lo[0]; x <= hi[0]; ++x)
    for (i64 y = lo[1]; y <= hi[1]; ++y)
      for (i64 z = lo[2]; z <= hi[2]; ++z) {
        const auto value = num_test::wide_power(s, point(x, y, z));
        if (first || compare(value, least) < 0) least = value;
        first = false;
      }
  first = true;
  for (int corner = 0; corner < 8; ++corner) {
    const auto value = num_test::wide_power(s, point((corner & 1) ? hi[0] : lo[0], (corner & 2) ? hi[1] : lo[1],
                                                     (corner & 4) ? hi[2] : lo[2]));
    if (first || compare(value, most) > 0) most = value;
    first = false;
  }
  return {sign(least), sign(most)};
}

u64 next(u64& state) {  // splitmix64 : tirage deterministe, independant du profil
  u64 z = (state += 0x9E3779B97F4A7C15ull);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
  return z ^ (z >> 31);
}

// Signe de la coordonnee j du centre, c_j=(a_j D+N_j)/D avec D>0, et position par rapport au domaine.
int center_sign(const Sphere& s, int j) {
  const i128 c = i128{s.anchor().coordinates()[j]} * s.denominator() + s.numerator()[j];
  return (c > 0) - (c < 0);
}
bool center_far(const Sphere& s, int j) {  // c_j<-2 ou c_j>M+1 : plancher sature
  const i128 c = i128{s.anchor().coordinates()[j]} * s.denominator() + s.numerator()[j];
  const i128 d = s.denominator(), m = i128{1} << kCoordBits;
  return c < -2 * d || c > (m + 2) * d;
}
bool center_tie(const Sphere& s, int j) {  // c_j demi-entier : 2C = 0 mod D et C != 0 mod D
  const i128 c = i128{s.anchor().coordinates()[j]} * s.denominator() + s.numerator()[j];
  const i128 d = s.denominator();
  return (2 * c) % d == 0 && c % d != 0;
}
}  // namespace

// Fixtures gravees : contrat distinct du continu, gains stricts, centre negatif, centre lointain, voie Wide.
MHGP11_TEST(lattice_fixtures, 40) {
  const Point zero = point(0, 0, 0);
  // Revue 10 : q2 de (0,0,0) a (1,0,0), segment [0,1]x{0}x{0}. F vaut 0 aux deux sites, -1/2 D au milieu : le
  // minorant entier est un contact (0), le minorant continu est negatif. Les deux bornes laissent raffiner.
  const auto unit = sphere(2, {zero, point(1, 0, 0), zero, zero});
  const Box segment = box(zero, point(1, 0, 0));
  const LatticeSphere unit_lattice(unit);
  CHECK(unit_lattice.lattice());
  const auto lattice_segment = unit_lattice.bound_signs(segment);
  const auto continuous_segment = power_bound_signs(unit, segment);
  REQUIRE(lattice_segment.ok() && continuous_segment.ok());
  CHECK_EQ(lattice_segment.value().lower, 0);  // le centre (1/2,0,0) est un ex aequo : point entier 0
  CHECK_EQ(lattice_segment.value().upper, 0);
  CHECK_EQ(continuous_segment.value().lower, -1);
  CHECK(center_tie(unit, 0));
  // Exclusion gagnee : q2 (0,0,0)-(4,0,0), c=(2,0,0), r=2 ; boite [3,5]x[2,3]x{0}, distance^2 = 1+4 = 5 > 4.
  const auto pair = sphere(2, {zero, point(4, 0, 0), zero, zero});
  const LatticeSphere pair_lattice(pair);
  const Box beside = box(point(3, 2, 0), point(5, 3, 0));
  const auto out_new = pair_lattice.bound_signs(beside);
  const auto out_old = power_bound_signs(pair, beside);
  REQUIRE(out_new.ok() && out_old.ok());
  CHECK_EQ(out_new.value().lower, 1);
  CHECK_EQ(out_new.value().upper, 1);
  CHECK(out_old.value().lower <= 0);  // la borne continue separee ne l'exclut pas
  // Inclusion gagnee : q2 (0,0,0)-(8,0,0), c=(4,0,0), r=4 ; boite [2,6]x[0,1]x[0,1], coin eloigne a distance^2 6.
  const auto wide_pair = sphere(2, {zero, point(8, 0, 0), zero, zero});
  const Box core = box(point(2, 0, 0), point(6, 1, 1));
  const auto in_new = LatticeSphere(wide_pair).bound_signs(core);
  const auto in_old = power_bound_signs(wide_pair, core);
  REQUIRE(in_new.ok() && in_old.ok());
  CHECK_EQ(in_new.value().lower, -1);
  CHECK_EQ(in_new.value().upper, -1);
  CHECK(in_old.value().upper >= 0);
  // Centre negatif : q3 (10,5,0), (9,0,0), (9,10,0), centre (-3,5,0), r=13. Boite [0,2]x[4,6]x{0} interieure.
  const auto obtuse = sphere(3, {point(10, 5, 0), point(9, 0, 0), point(9, 10, 0), zero});
  CHECK_EQ(center_sign(obtuse, 0), -1);
  const auto inside_negative = LatticeSphere(obtuse).bound_signs(box(point(0, 4, 0), point(2, 6, 0)));
  REQUIRE(inside_negative.ok());
  CHECK_EQ(inside_negative.value().upper, -1);
  const auto truth_negative = truth(obtuse, box(point(0, 4, 0), point(2, 6, 0)));
  CHECK_EQ(truth_negative.upper, -1);
  // Contact sur le cercle : (10,5,0) est un support ; boite [10,11]x[5,5]x{0}, minimum 0 au support.
  const auto contact = LatticeSphere(obtuse).bound_signs(box(point(10, 5, 0), point(11, 5, 0)));
  REQUIRE(contact.ok());
  CHECK_EQ(contact.value().lower, 0);
  CHECK_EQ(contact.value().upper, 1);
  // Centres lointains (plancher sature des deux cotes) : triangles presque alignes, centre a |c_y| > 2^24.
  const i64 k = 1000;
  const auto below = sphere(3, {point(0, k, 0), point(20000, k + 1, 0), point(40000, k, 0), zero});
  const auto above = sphere(3, {point(0, k + 1, 0), point(20000, k, 0), point(40000, k + 1, 0), zero});
  CHECK(center_far(below, 1));
  CHECK(center_far(above, 1));
  CHECK_EQ(center_sign(below, 1), -1);  // le centre est du cote oppose au sommet obtus : y = k - 2.10^8 environ
  CHECK_EQ(center_sign(above, 1), 1);   // y = k + 2.10^8 environ, au-dela de M aux trois profils
  for (const Sphere* far : {&below, &above}) {
    for (const Box b : {box(point(19998, k - 2, 0), point(20002, k + 3, 2)), box(point(0, 0, 0), point(3, 3, 3)),
                        box(point(39999, k, 0), point(40001, k + 1, 1))}) {
      const auto signs = LatticeSphere(*far).bound_signs(b);
      REQUIRE(signs.ok());
      const auto expected = truth(*far, b);
      CHECK_EQ(signs.value().lower, expected.lower);
      if (expected.lower <= 0) CHECK_EQ(signs.value().upper, expected.upper);
    }
  }
  // Voie Wide : q3 aux coins du domaine, D=6(M-1)^4 hors certificat q3 aux profils 21 et 24 ; meme borne qu'avant.
  const i64 m = kCoordMax;
  const auto large = sphere(3, {zero, point(m, m, 0), point(m, 0, m), zero});
  const LatticeSphere large_lattice(large);
  CHECK_EQ(large_lattice.lattice(), kCoordBits == 18);
  for (const Box b : {box(zero, point(3, 3, 3)), box(point(m - 3, m - 3, 0), point(m, m, 2)),
                      box(zero, point(m, m, m))}) {
    const auto signs = large_lattice.bound_signs(b);
    const auto continuous = power_bound_signs(large, b);
    REQUIRE(signs.ok() && continuous.ok());
    if (!large_lattice.lattice()) {
      CHECK_EQ(signs.value().lower, continuous.value().lower);
      CHECK_EQ(signs.value().upper, continuous.value().upper);
    } else {
      CHECK(continuous.value().lower <= 0 || signs.value().lower > 0);
      CHECK(continuous.value().upper >= 0 || signs.value().upper < 0);
    }
  }
}

// Tirages : quatre arites, trois regions (origine, milieu, coin maximal), boites jusqu'a 5x5x5 points entiers.
MHGP11_TEST(lattice_random, 200000) {
  u64 state = 0x6C61747469636531ull;
  u64 spheres = 0, boxes = 0, outside = 0, inside = 0, contacts = 0, gained_out = 0, gained_in = 0;
  u64 negative = 0, far = 0, ties = 0, sides = 0;
  const i64 m = kCoordMax;
  for (int trial = 0; trial < 6000; ++trial) {
    const int region = trial % 3;
    const i64 span = 4 + static_cast<i64>(next(state) % 13);  // supports dans un cube de cote 4..16
    const i64 base = region == 0 ? 0 : region == 1 ? (m / 2) : m - span;
    std::array<Point, 4> support{};
    for (auto& p : support)
      p = point(base + static_cast<i64>(next(state) % static_cast<u64>(span + 1)),
                base + static_cast<i64>(next(state) % static_cast<u64>(span + 1)),
                base + static_cast<i64>(next(state) % static_cast<u64>(span + 1)));
    const u8 q = static_cast<u8>(1 + trial % 4);
    const auto made = through(q, support);
    if (!made) continue;
    const Sphere& s = *made;
    ++spheres;
    for (int j = 0; j < 3; ++j) {
      negative += center_sign(s, j) < 0;
      far += center_far(s, j);
      ties += center_tie(s, j);
    }
    const LatticeSphere lattice(s);
    CHECK(lattice.lattice());  // supports locaux : centre natif a tout profil
    for (int draw = 0; draw < 6; ++draw) {
      std::array<i64, 3> lo{}, hi{};
      for (int j = 0; j < 3; ++j) {
        const i64 start = base - 3 + static_cast<i64>(next(state) % static_cast<u64>(span + 7));
        lo[j] = std::clamp<i64>(start, 0, m);
        hi[j] = std::min<i64>(m, lo[j] + static_cast<i64>(next(state) % 5));
      }
      const Box b = box(point(lo[0], lo[1], lo[2]), point(hi[0], hi[1], hi[2]));
      const auto signs = lattice.bound_signs(b);
      const auto old = power_bound_signs(s, b);
      REQUIRE(signs.ok() && old.ok());
      const auto expected = truth(s, b);
      ++boxes;
      CHECK_EQ(signs.value().lower, expected.lower);
      if (expected.lower <= 0) CHECK_EQ(signs.value().upper, expected.upper);
      else CHECK_EQ(signs.value().upper, 1);
      CHECK(signs.value().lower <= signs.value().upper);
      // Monotonie : toute decision de la borne continue est prise aussi par la borne entiere.
      CHECK(old.value().lower <= 0 || signs.value().lower > 0);
      CHECK(old.value().upper >= 0 || signs.value().upper < 0);
      outside += signs.value().lower > 0;
      inside += signs.value().upper < 0;
      contacts += signs.value().lower == 0;
      gained_out += signs.value().lower > 0 && old.value().lower <= 0;
      gained_in += signs.value().upper < 0 && old.value().upper >= 0;
      const Point probe = point(lo[0] + static_cast<i64>(next(state) % static_cast<u64>(hi[0] - lo[0] + 1)), lo[1], hi[2]);
      const auto mine = lattice.side(probe);
      const auto reference = num::side(s, probe);
      REQUIRE(mine.ok() && reference.ok());
      CHECK_EQ(mine.value(), reference.value());
      ++sides;
    }
  }
  std::printf("lattice_random spheres=%llu boxes=%llu outside=%llu inside=%llu contacts=%llu gained_out=%llu "
              "gained_in=%llu negative=%llu far=%llu ties=%llu sides=%llu\n",
              static_cast<unsigned long long>(spheres), static_cast<unsigned long long>(boxes),
              static_cast<unsigned long long>(outside), static_cast<unsigned long long>(inside),
              static_cast<unsigned long long>(contacts), static_cast<unsigned long long>(gained_out),
              static_cast<unsigned long long>(gained_in), static_cast<unsigned long long>(negative),
              static_cast<unsigned long long>(far), static_cast<unsigned long long>(ties),
              static_cast<unsigned long long>(sides));
  // Planchers de couverture, fixes sous les valeurs observees aux trois profils (vert par vacuite refuse).
  CHECK(spheres >= 5000 && boxes >= 30000 && sides >= 30000);
  CHECK(outside >= 20000 && inside >= 1500 && contacts >= 250);
  CHECK(gained_out >= 2000 && gained_in >= 600);
  CHECK(negative >= 100 && far >= 100 && ties >= 2000);
}

MHGP11_TEST_MAIN()
