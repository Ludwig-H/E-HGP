// Invariance par translation des predicats en repere local (docs/CONTRAT_NUMERIQUE.md, paragraphe 7), jugee a
// translation pres : supports q1 a q4 tires pres de l'origine, puis translates jusqu'au bord superieur du domaine du
// profil (au profil 32, [0, 2^32)) ; niveaux, certificats de domaine, etendues, certification, cotes, orientations,
// milieux, ordre des centres, garde et bornes entieres identiques.
#include <array>
#include <cstdio>

#include "local_reference.hpp"
#include "test.hpp"

using namespace mhgp12;
using namespace mhgp12::num;
using local_test::point;

namespace {
u64 next(u64& state) {  // splitmix64
  u64 z = (state += 0x9E3779B97F4A7C15ull);
  z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
  z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
  return z ^ (z >> 31);
}
using Triple = std::array<i64, 3>;
Point moved(const Triple& p, const Triple& shift) { return point(p[0] + shift[0], p[1] + shift[1], p[2] + shift[2]); }
std::optional<Sphere> through(const std::array<Triple, 4>& p, u8 q, const Triple& shift) {
  const auto a = moved(p[0], shift), b = moved(p[1], shift), c = moved(p[2], shift), d = moved(p[3], shift);
  if (q == 1) return Sphere::point(a);
  const auto made = q == 2 ? Sphere::through(a, b) : q == 3 ? Sphere::through(a, b, c) : Sphere::through(a, b, c, d);
  if (!made.ok()) throw std::runtime_error("sphere de translation refusee");
  return made.value();
}
std::optional<CertifiedBall> certify(const std::array<Triple, 4>& p, u8 q, const Triple& shift) {
  std::vector<Point> support;
  for (u8 i = 0; i < q; ++i) support.push_back(moved(p[i], shift));
  const auto made = CertifiedBall::certify(support);
  if (!made.ok()) throw std::runtime_error("certificat de translation refuse");
  return made.value();
}
}  // namespace

MHGP12_TEST(translation, 20000) {
  u64 state = 0x7472616E736C6174ull;
  const i64 top = kCoordMax;
  u64 spheres = 0, certified = 0, queries = 0, boxes = 0, wide = 0, orders = 0;
  for (int trial = 0; trial < 1200; ++trial) {
    const i64 span = trial % 5 == 4 ? (i64{1} << 20) : 4 + static_cast<i64>(next(state) % 29);
    std::array<Triple, 4> p{};
    for (auto& v : p)
      for (auto& x : v) x = static_cast<i64>(next(state) % static_cast<u64>(span + 1));
    const u8 q = static_cast<u8>(1 + trial % 4);
    // Translation vers le bord superieur du domaine du profil, ou a mi-domaine.
    i64 high = 0;
    for (const auto& v : p) high = std::max({high, v[0], v[1], v[2]});
    const Triple shift = trial % 2 == 0 ? Triple{top - high, top - high, top - high}
                                        : Triple{top / 2, top - high, static_cast<i64>(next(state) % 1024)};
    const auto origin = through(p, q, {0, 0, 0}), far = through(p, q, shift);
    REQUIRE(origin.has_value() == far.has_value());
    if (!origin) continue;
    ++spheres;
    CHECK_EQ(compare(origin->level(), far->level()), 0);
    CHECK_EQ(origin->support_span(), far->support_span());
    CHECK_EQ(origin->power_domain(), far->power_domain());
    CHECK_EQ(origin->orientation_domain(), far->orientation_domain());
    CHECK(origin->numerator() == far->numerator() && origin->denominator() == far->denominator());
    const auto ball = certify(p, q, {0, 0, 0}), far_ball = certify(p, q, shift);
    REQUIRE(ball.has_value() == far_ball.has_value());
    if (ball) {
      ++certified;
      CHECK_EQ(ball->span(), far_ball->span());
    }
    for (int draw = 0; draw < 6; ++draw) {
      Triple z{};  // dans la boite des supports, coins de boite compris : la translation reste dans le domaine
      for (auto& x : z) x = static_cast<i64>(next(state) % static_cast<u64>(std::max<i64>(high - 2, 1)));
      LaneCount lanes;
      const auto here = side(*origin, point(z[0], z[1], z[2]), &lanes), there = side(*far, moved(z, shift));
      REQUIRE(here.ok() && there.ok());
      CHECK_EQ(here.value(), there.value());
      const auto a = point(z[0], z[1], 0), b = point(0, z[1], z[2]);
      CHECK_EQ(is_midpoint(*origin, a, b), is_midpoint(*far, moved({z[0], z[1], 0}, shift), moved({0, z[1], z[2]}, shift)));
      const auto o = orientation(a, b, point(z[0], 0, z[2]), *origin);
      const auto o2 = orientation(moved({z[0], z[1], 0}, shift), moved({0, z[1], z[2]}, shift),
                                  moved({z[0], 0, z[2]}, shift), *far);
      REQUIRE(o.ok() && o2.ok());
      CHECK_EQ(o.value(), o2.value());
      const Triple hi{z[0] + 3, z[1] + 2, z[2] + 1};
      const auto box_here = Box::make(point(z[0], z[1], z[2]), point(hi[0], hi[1], hi[2]));
      const auto box_there = Box::make(moved(z, shift), moved(hi, shift));
      REQUIRE(box_here.ok() && box_there.ok());
      const LatticeSphere lattice(*origin), far_lattice(*far);
      const auto s1 = lattice.bound_signs(box_here.value()), s2 = far_lattice.bound_signs(box_there.value());
      REQUIRE(s1.ok() && s2.ok());
      CHECK(lattice.lattice() == far_lattice.lattice());
      CHECK(s1.value().lower == s2.value().lower && s1.value().upper == s2.value().upper);
      if (ball) {
        const GuardedSphere guard(*ball), far_guard(*far_ball);
        const auto g1 = guard.bound_signs(box_here.value()), g2 = far_guard.bound_signs(box_there.value());
        const auto t1 = guard.side(point(z[0], z[1], z[2])), t2 = far_guard.side(moved(z, shift));
        REQUIRE(g1.ok() && g2.ok() && t1.ok() && t2.ok());
        CHECK(g1.value().lower == g2.value().lower && g1.value().upper == g2.value().upper);
        CHECK_EQ(t1.value(), t2.value());
        CHECK_EQ(t1.value(), here.value());
      }
      ++queries;
      ++boxes;
      wide += lanes.total() - lanes.native;  // voies hors palier etroit : grands supports
    }
    // Ordre des centres : le meme pour deux boules translatees ensemble (second support du meme tirage, meme
    // translation, contenu dans la boite du premier).
    std::array<Triple, 4> other{};
    for (auto& v : other)
      for (auto& x : v) x = static_cast<i64>(next(state) % static_cast<u64>(high + 1));
    const auto second = through(other, static_cast<u8>(1 + (trial / 4) % 4), {0, 0, 0});
    const auto second_far = through(other, static_cast<u8>(1 + (trial / 4) % 4), shift);
    if (second && second_far) {
      CHECK_EQ(compare_centers(*origin, *second), compare_centers(*far, *second_far));
      CHECK_EQ(compare_centers(*second, *origin), compare_centers(*second_far, *far));
      ++orders;
    }
  }
  std::printf("translation spheres=%llu certified=%llu queries=%llu boxes=%llu nonnative=%llu orders=%llu\n",
              static_cast<unsigned long long>(spheres), static_cast<unsigned long long>(certified),
              static_cast<unsigned long long>(queries), static_cast<unsigned long long>(boxes),
              static_cast<unsigned long long>(wide), static_cast<unsigned long long>(orders));
  CHECK(spheres >= 1000 && certified >= 300 && queries >= 6000 && wide > 0 && orders >= 800);
}
