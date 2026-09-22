#include "core/float32_edge_geometry.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

#include "core/fixed_signed.hpp"

namespace mhgp8 {
namespace {
using I = float32_predicate_detail::Interval;
using F = float32_ball_detail::FixedSigned;
using float32_predicate_detail::as_double;
using float32_predicate_detail::count;
using float32_predicate_detail::down;
using float32_predicate_detail::up;
template<class T> using V = std::array<T, 3>;

struct IntervalOps {
  Float32EdgeWork& w;
  I add(I a, I b) { count(w.interval_additions); return float32_predicate_detail::add(a, b); }
  I sub(I a, I b) { return add(a, {-b.high, -b.low}); }
  I mul(I a, I b) { count(w.interval_products); return float32_predicate_detail::multiply(a, b); }
  I square(I a) {
    count(w.interval_products);
    const double lo = a.low <= 0 && a.high >= 0 ? 0 : std::min(std::abs(a.low), std::abs(a.high));
    const double hi = std::max(std::abs(a.low), std::abs(a.high));
    return {std::max(0.0, down(lo * lo)), up(hi * hi)};
  }
};
struct ExactOps {
  Float32EdgeWork& w;
  F add(const F& a, const F& b) { count(w.exact_additions); return a + b; }
  F sub(const F& a, const F& b) { count(w.exact_additions); return a - b; }
  F mul(const F& a, const F& b) { count(w.exact_products); return a * b; }
  F square(const F& a) { return mul(a, a); }
};
template<class O, class T> V<T> sub(O& op, const V<T>& a, const V<T>& b) {
  return {op.sub(a[0], b[0]), op.sub(a[1], b[1]), op.sub(a[2], b[2])};
}
template<class O, class T> T dot(O& op, const V<T>& a, const V<T>& b) {
  return op.add(op.add(op.mul(a[0], b[0]), op.mul(a[1], b[1])), op.mul(a[2], b[2]));
}
template<class O, class T> T norm(O& op, const V<T>& v) {
  return op.add(op.add(op.square(v[0]), op.square(v[1])), op.square(v[2]));
}
template<class O, class T> V<T> cross(O& op, const V<T>& a, const V<T>& b) {
  return {op.sub(op.mul(a[1], b[2]), op.mul(a[2], b[1])),
          op.sub(op.mul(a[2], b[0]), op.mul(a[0], b[2])),
          op.sub(op.mul(a[0], b[1]), op.mul(a[1], b[0]))};
}
V<I> vector(const Float32Point3& p) {
  V<I> out{};
  for (std::size_t i = 0; i != 3; ++i) {
    const auto x = as_double(p.bits()[i]);
    out[i] = {x, x};
  }
  return out;
}
V<I> vector(const Float32Box3& p) {
  V<I> out{};
  for (std::size_t i = 0; i != 3; ++i)
    out[i] = {as_double(p.low()[i]), as_double(p.high()[i])};
  return out;
}
V<F> exact(const Float32Words& p, Float32EdgeWork& w) {
  count(w.exact_decodes, 3);
  return {F::from_bits(p[0]), F::from_bits(p[1]), F::from_bits(p[2])};
}
bool singleton(const Float32Box3& p) {
  for (std::size_t i = 0; i != 3; ++i)
    if (float32_order_key(p.low()[i]) != float32_order_key(p.high()[i])) return false;
  return true;
}
bool same(const Float32Point3& a, const Float32Point3& b) {
  for (std::size_t i = 0; i != 3; ++i)
    if (float32_order_key(a.bits()[i]) != float32_order_key(b.bits()[i])) return false;
  return true;
}
void validate_mode(Float32PredicateMode mode) {
  if (mode != Float32PredicateMode::Filtered && mode != Float32PredicateMode::ExactOnly)
    throw std::invalid_argument("mhgp8 edge geometry invalid predicate mode");
}
int interval_citron(I h, I xi, IntervalOps& op) {
  if (h.high <= 0) return 1;
  const auto lhs = op.mul({3, 3}, op.square(h));
  if (h.low > 0 && lhs.low > xi.high) return -1;
  // h.high>0, so the largest possible positive H is h.high, regardless of
  // a negative h.low. Squaring the whole interval here would lose exclusion.
  const auto upper = op.mul({3, 3}, op.square({h.high, h.high}));
  if (upper.high <= xi.low) return 1;
  return 0;
}
int fixed_box_citron(const V<I>& av, const V<I>& delta, I diameter,
                     const V<I>& zv, IntervalOps& op) {
  const auto r = sub(op, zv, av);
  V<I> relative_mid{};
  for (std::size_t i = 0; i != 3; ++i) relative_mid[i] = op.mul(delta[i], {0.5, 0.5});
  // H=D/4-|z-(a+b)/2|^2. This encloses the continuous interior vertex, not
  // merely the corners of Z. Xi uses the affine fixed-edge cross product.
  const auto h = op.sub(op.mul(diameter, {0.25, 0.25}), norm(op, sub(op, r, relative_mid)));
  return interval_citron(h, norm(op, cross(op, delta, r)), op);
}
bool exact_citron(const Float32Point3& a, const Float32Point3& b,
                 const Float32Point3& z, Float32EdgeWork& work) {
  count(work.citron_exact_queries);
  ExactOps op{work};
  const auto av = exact(a.bits(), work);
  const auto d = sub(op, exact(b.bits(), work), av);
  const auto r = sub(op, exact(z.bits(), work), av);
  const auto h = dot(op, r, sub(op, d, r));
  if (h.sign() <= 0) return false;
  return op.sub(op.mul(F::from_unsigned(3), op.square(h)), norm(op, cross(op, d, r))).sign() > 0;
}
int record_citron(int value, Float32EdgeWork& work) {
  if (value < 0) count(work.citron_inside);
  else if (value > 0) count(work.citron_outside);
  else count(work.citron_unknown);
  return value;
}
std::array<std::size_t, 2> pair(std::size_t a, std::size_t b) {
  return a < b ? std::array{a, b} : std::array{b, a};
}
}  // namespace

Float32EdgeGeometry Float32EdgeGeometry::make(
    const Float32Point3& a, std::size_t a_id, const Float32Point3& b, std::size_t b_id,
    Float32PredicateMode mode, Float32EdgeWork& work) {
  validate_mode(mode);
  if (a_id == b_id || same(a, b))
    throw std::invalid_argument("mhgp8 edge geometry requires distinct endpoints");
  count(work.preparations);
  IntervalOps op{work};
  const auto delta = sub(op, vector(b), vector(a));
  return Float32EdgeGeometry({a, b}, {a_id, b_id}, mode, delta, norm(op, delta));
}

bool Float32EdgeGeometry::owns(const Float32Point3& x, std::size_t x_id, Float32EdgeWork& work) const {
  count(work.owner_queries);
  if (x_id == ids_[0] || x_id == ids_[1]) {
    count(work.owner_rejections);
    return false;
  }
  if (mode_ == Float32PredicateMode::Filtered) {
    IntervalOps op{work};
    const auto ax = op.sub(norm(op, sub(op, vector(x), vector(points_[0]))), diameter_);
    const auto bx = op.sub(norm(op, sub(op, vector(x), vector(points_[1]))), diameter_);
    if (ax.low > 0 || bx.low > 0) {
      count(work.owner_filter_accepts);
      count(work.owner_rejections);
      return false;
    }
    if (ax.high < 0 && bx.high < 0) {
      count(work.owner_filter_accepts);
      return true;
    }
  }
  count(work.owner_exact_queries);
  ExactOps op{work};
  const auto a = exact(points_[0].bits(), work), b = exact(points_[1].bits(), work);
  const auto z = exact(x.bits(), work);
  const auto d = norm(op, sub(op, b, a));
  const auto sa = op.sub(norm(op, sub(op, z, a)), d).sign();
  const auto sb = op.sub(norm(op, sub(op, z, b)), d).sign();
  const auto ab = pair(ids_[0], ids_[1]);
  if (sa == 0) count(work.owner_equalities);
  if (sb == 0) count(work.owner_equalities);
  const bool result = (sa < 0 || (sa == 0 && ab < pair(ids_[0], x_id))) &&
                      (sb < 0 || (sb == 0 && ab < pair(ids_[1], x_id)));
  if (!result) count(work.owner_rejections);
  return result;
}

bool Float32EdgeGeometry::may_own(const Float32Box3& x, Float32EdgeWork& work) const {
  count(work.owner_box_queries);
  IntervalOps op{work};
  const auto ax = norm(op, sub(op, vector(x), vector(points_[0])));
  const auto bx = norm(op, sub(op, vector(x), vector(points_[1])));
  const bool rejected = ax.low > diameter_.high || bx.low > diameter_.high ||
                        op.add(ax, bx).high <= diameter_.low;
  if (rejected) count(work.owner_box_rejections);
  return !rejected;
}

bool Float32EdgeGeometry::citron(const Float32Point3& z, Float32EdgeWork& work) const {
  count(work.citron_point_queries);
  if (mode_ == Float32PredicateMode::Filtered) {
    IntervalOps op{work};
    const auto status = fixed_box_citron(vector(points_[0]), delta_, diameter_, vector(z), op);
    if (status != 0) {
      count(work.citron_filter_accepts);
      return status < 0;
    }
  }
  return exact_citron(points_[0], points_[1], z, work);
}

int Float32EdgeGeometry::citron(const Float32Box3& z, Float32EdgeWork& work) const {
  count(work.citron_box_queries);
  IntervalOps op{work};
  int status = fixed_box_citron(vector(points_[0]), delta_, diameter_, vector(z), op);
  if (status == 0 && singleton(z)) status = citron(Float32Point3::from_bits(z.low()), work) ? -1 : 1;
  return record_citron(status, work);
}

int float32_q3_citron_boxes(const Float32Box3& a, const Float32Box3& b,
                           const Float32Box3& z, Float32EdgeWork& work) {
  count(work.citron_box_queries);
  IntervalOps op{work};
  const auto av = vector(a), bv = vector(b), zv = vector(z);
  const auto r = sub(op, zv, av), d = sub(op, bv, av);
  const auto h = dot(op, r, sub(op, bv, zv));
  int status = interval_citron(h, norm(op, cross(op, d, r)), op);
  if (status == 0 && singleton(a) && singleton(b) && singleton(z)) {
    count(work.citron_point_queries);
    status = exact_citron(Float32Point3::from_bits(a.low()), Float32Point3::from_bits(b.low()),
                         Float32Point3::from_bits(z.low()), work) ? -1 : 1;
  }
  return record_citron(status, work);
}

bool float32_boxes_separated(const Float32Box3& a, const Float32Box3& b,
                            std::uint32_t s, Float32EdgeWork& work) {
  if (s == 0) throw std::invalid_argument("mhgp8 float32 separation must be positive");
  count(work.separation_queries);
  IntervalOps op{work};
  const auto av = vector(a), bv = vector(b);
  V<I> wa{}, wb{}, gap{};
  for (std::size_t i = 0; i != 3; ++i) {
    wa[i] = op.sub({av[i].high, av[i].high}, {av[i].low, av[i].low});
    wb[i] = op.sub({bv[i].high, bv[i].high}, {bv[i].low, bv[i].low});
    if (av[i].low > bv[i].high) gap[i] = op.sub({av[i].low, av[i].low}, {bv[i].high, bv[i].high});
    else if (bv[i].low > av[i].high) gap[i] = op.sub({bv[i].low, bv[i].low}, {av[i].high, av[i].high});
    else gap[i] = {0, 0};
  }
  const auto da = norm(op, wa), db = norm(op, wb);
  const auto scale = op.square({static_cast<double>(s), static_cast<double>(s)});
  const auto difference = op.sub(norm(op, gap), op.mul(scale, {std::max(da.low, db.low), std::max(da.high, db.high)}));
  if (difference.low > 0 || difference.high < 0) {
    count(work.separation_filter_accepts);
    return difference.low > 0;
  }
  count(work.separation_exact_queries);
  ExactOps ex{work};
  const auto al = exact(a.low(), work), ah = exact(a.high(), work);
  const auto bl = exact(b.low(), work), bh = exact(b.high(), work);
  V<F> eg{};
  for (std::size_t i = 0; i != 3; ++i) {
    if (al[i].compare(bh[i]) > 0) eg[i] = ex.sub(al[i], bh[i]);
    else if (bl[i].compare(ah[i]) > 0) eg[i] = ex.sub(bl[i], ah[i]);
  }
  const auto ea = norm(ex, sub(ex, ah, al)), eb = norm(ex, sub(ex, bh, bl));
  const auto largest = ea.compare(eb) >= 0 ? ea : eb;
  return ex.sub(norm(ex, eg), ex.mul(ex.square(F::from_unsigned(s)), largest)).sign() >= 0;
}

// In units2^-149, coordinate differences <M=2^278. Degree2 quantities are
// <12M^2; citron H^2 and Xi plus subtraction <48M^4<2^1118. Separation adds
// at most64bits for s^2 (<2^64), well below1728. No unreduced degree9 product.
// Physical interval polynomials have degree<=4 and magnitude<2^523; no
// infinity or subnormal operand escapes the outward-normalized operations.
}  // namespace mhgp8
