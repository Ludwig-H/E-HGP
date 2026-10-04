// Essais controles : limites synthetiques et spheres fermees contre Wide, y compris annulations intermediaires.
#include <algorithm>
#include <array>

#include "checked_power_support.hpp"
#include "test.hpp"

using namespace mhgp11;
using namespace mhgp11::num;

namespace {
Point point(i64 x, i64 y, i64 z) {
  const auto made = Point::make(x, y, z);
  if (!made.ok()) throw std::runtime_error("checked fixture hors domaine");
  return made.value();
}
Sphere sphere(const std::array<Point, 3>& points) {
  const auto made = Sphere::through(points[0], points[1], points[2]);
  if (!made.ok() || !made.value()) throw std::runtime_error("checked fixture degeneree");
  return *made.value();
}
void judge(const Sphere& s, Point p, Point lo, Point hi) {
  const auto box = Box::make(lo, hi);
  REQUIRE(box.ok());
  const auto actual = power(s, p);
  const auto sign = side(s, p);
  const auto bounds = power_bounds(s, box.value());
  REQUIRE(actual.ok() && sign.ok() && bounds.ok());
  CHECK(num_test::equals_wide(actual.value(), num_test::wide_power(s, p)));
  CHECK_EQ(sign.value(), num_test::wide_power(s, p).sign());
  const auto terms = checked_test::box_terms(s, box.value());
  CHECK(num_test::equals_wide(bounds.value().lower, checked_test::wide_sum(s, terms[0])));
  CHECK(num_test::equals_wide(bounds.value().upper, checked_test::wide_sum(s, terms[1])));
}
bool fits(const Sphere& s, Point p) {
  return checked_test::attempt(s, checked_test::query_terms(s, p)).has_value();
}
std::array<bool, 2> fits_box(const Sphere& s, Point lo, Point hi) {
  const auto box = Box::make(lo, hi);
  if (!box.ok()) throw std::runtime_error("checked fixture boite inversee");
  const auto terms = checked_test::box_terms(s, box.value());
  return {checked_test::attempt(s, terms[0]).has_value(), checked_test::attempt(s, terms[1]).has_value()};
}
}  // namespace

MHGP11_TEST(checked_limits, 12) {
  const i128 maximum = static_cast<i128>((u128{1} << 127) - 1), minimum = -maximum - 1;
  const auto run = [](i128 d, std::array<i128, 3> n, i64 norm, std::array<i64, 3> f) {
    return mhgp11::num::detail::checked_power_sum(d, n, norm, f);
  };
  CHECK(run(0, {}, 0, {}) == std::optional<i128>{0});
  CHECK(run(maximum, {}, 1, {}) == std::optional<i128>{maximum});
  CHECK(run(minimum, {}, 1, {}) == std::optional<i128>{minimum});
  CHECK(!run(maximum, {}, 2, {}));
  CHECK(!run(minimum, {}, -1, {}));
  CHECK(!run(0, {maximum, 0, 0}, 0, {2, 0, 0}));
  CHECK(!run(maximum, {1, 0, 0}, 1, {1, 0, 0}));
  CHECK(!run(minimum, {1, 0, 0}, 1, {-1, 0, 0}));
  CHECK(run(maximum, {maximum, 0, 0}, 1, {-1, 0, 0}) == std::optional<i128>{0});
  CHECK(run(0, {minimum, 0, 0}, 0, {1, 0, 0}) == std::optional<i128>{minimum});
  CHECK(!run(0, {minimum, 0, 0}, 0, {-1, 0, 0}));
  CHECK(!run(maximum, {maximum, 0, 0}, 2, {-2, 0, 0}));  // Final0 ne certifie pas les produits.
}

MHGP11_TEST(checked_public, 250) {
  const i64 m = kCoordMax;
  const auto zero = point(0, 0, 0), top = point(m, m, m);
  std::array<u32, 3> order{0, 1, 2};
  for (const i64 scale : {i64{4}, m}) {
    const std::array<Point, 3> pts{zero, point(scale, scale, 0), point(scale, 0, scale)};
    do {
      const auto s = sphere({pts[order[0]], pts[order[1]], pts[order[2]]});
      for (const auto p : {zero, pts[1], point(0, scale, scale)}) judge(s, p, zero, top);
    } while (std::next_permutation(order.begin(), order.end()));
  }
  const auto small = sphere({zero, point(4, 0, 0), point(0, 4, 0)});
  CHECK(fits(small, top));
  CHECK(fits_box(small, zero, top)[0]); CHECK(fits_box(small, zero, top)[1]);
  judge(small, zero, zero, zero);  // Contact exactement nul.
  const auto large = sphere({zero, point(m, m, 0), point(m, 0, m)});
  CHECK_EQ(fits_box(large, zero, top)[0], kCoordBits == 18);
  CHECK_EQ(fits_box(large, zero, top)[1], kCoordBits == 18);
  CHECK_EQ(fits(large, point(0, m, m)), kCoordBits == 18);
  CHECK_EQ(fits(large, point(m, m, 0)), kCoordBits == 18);  // H=0, premier produit trop grand.
  judge(large, point(m, m, 0), point(m, m, 0), point(m, m, 0));
  const i64 l = std::min(m, i64{2097151}), t = std::min(m, i64{1200000});
  const auto lower = sphere({zero, point(l, l, 0), point(l, 0, l)});
  const auto lower_fit = fits_box(lower, zero, point(0, t, 0));
  CHECK_EQ(lower_fit[0], kCoordBits == 18); CHECK(lower_fit[1]);
  judge(lower, zero, zero, point(0, t, 0));
  const i64 local = std::min(m, i64{524287});
  const auto upper = sphere({zero, point(local, local, 0), point(local, 0, local)});
  const auto upper_fit = fits_box(upper, zero, top);
  CHECK(upper_fit[0]); CHECK_EQ(upper_fit[1], kCoordBits != 24);
  judge(upper, top, zero, top);  // Support local19bits ne borne PAS le temoin global24bits.
  const i64 u = std::min(m, i64{1250000}), h = std::min(m, i64{2097151});
  const auto asymmetric = sphere({zero, point(u, u, 0), point(u, 0, u)});
  const auto asymmetric_fit = fits_box(asymmetric, zero, point(h, h, h));
  CHECK(asymmetric_fit[0]); CHECK_EQ(asymmetric_fit[1], kCoordBits == 18);
  judge(asymmetric, zero, zero, point(h, h, h));
  if constexpr (kCoordBits >= 21) {
    const i64 s = 269568;
    const auto cancellation = sphere({point(2*s, 0, 7*s), point(s, 6*s, 2*s), point(7*s, 3*s, s)});
    CHECK(!fits(cancellation, point(0, 7*s, s)));  // Tous produits tiennent, somme partielle deborde.
    judge(cancellation, point(0, 7*s, s), zero, top);
  }
  if constexpr (kCoordBits == 24) {
    const i64 s = 476192;
    const auto addition = sphere({point(3*s, 2*s, 7*s), point(7*s, 6*s, 8*s), point(3*s, 6*s, 3*s)});
    CHECK(!fits(addition, point(s, 2*s, 5*s)));
    judge(addition, point(s, 2*s, 5*s), zero, top);
  }
}
