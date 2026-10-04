#include "arith/geometry.hpp"

#include <cmath>

namespace mhgp10::geom {

namespace {
double to_double(const arith::I192& v) {
  double d = 0;
  for (int i = 2; i >= 0; --i) d = d * 18446744073709551616.0 + static_cast<double>(v.w[i]);
  return v.neg ? -d : d;
}
double to_double(const arith::I128w& v) {
  const double d = static_cast<double>(v.w[1]) * 18446744073709551616.0 + static_cast<double>(v.w[0]);
  return v.neg ? -d : d;
}
arith::I192 u128_to_i192(u128 v) { return arith::I192::from_u128(v); }
}  // namespace

double Level::approx() const { return to_double(num) / to_double(den); }

int compare(const Level& a, const Level& b) {
  return arith::cmp(arith::mul(a.num, b.den), arith::mul(b.num, a.den));
}

bool strictly_inside_tetra(const P3* t[4], const P3& anchor, const Center& c) {
  for (int f = 0; f < 4; ++f) {
    const P3& p0 = *t[(f + 1) % 4];
    const P3& p1 = *t[(f + 2) % 4];
    const P3& p2 = *t[(f + 3) % 4];
    const int so = orient(p0, p1, p2, *t[f]);
    const int sc = orient_center(p0, p1, p2, anchor, c);
    if (sc == 0 || so != sc) return false;
  }
  return true;
}

int orient_center_wide(const P3& p, const P3& q, const P3& r, const P3& anchor, const Center& c) {
  const P3 a = sub(q, p), b = sub(r, p);
  const i128 w[3] = {i128(a.y) * b.z - i128(a.z) * b.y, i128(a.z) * b.x - i128(a.x) * b.z, i128(a.x) * b.y - i128(a.y) * b.x};
  const i128 cc[3] = {c.N[0] + c.D * (anchor.x - p.x), c.N[1] + c.D * (anchor.y - p.y), c.N[2] + c.D * (anchor.z - p.z)};
  arith::Wide<4> acc;
  for (int i = 0; i < 3; ++i) {
    const auto prod = arith::mul(arith::I128w::from_i128(w[i]), arith::I128w::from_i128(cc[i]));
    arith::Wide<4> next;
    arith::add(acc, prod, next);
    acc = next;
  }
  return acc.sign();
}

Level level2(const P3& a, const P3& b) {
  const P3 u = sub(b, a);
  Level l;
  l.num = u128_to_i192(static_cast<u128>(dot(u, u)));
  l.den = arith::I128w::from_u128(4);
  return l;
}

Level level3(const P3& a, const P3& b, const P3& c) {
  const P3 u = sub(b, a), v = sub(c, a), d = sub(c, b);
  const P3 w = cross(u, v);
  const u128 uu = static_cast<u128>(dot(u, u)), vv = static_cast<u128>(dot(v, v)), dd = static_cast<u128>(dot(d, d));
  const u128 ww = static_cast<u128>(i128(w.x) * w.x + i128(w.y) * w.y + i128(w.z) * w.z);
  Level l;
  // num = uu * vv * dd < 2^119 : produit en 192 bits
  const arith::I192 uv = arith::I192::from_u128(uu * vv);  // < 2^80
  arith::I192 r;
  arith::resize(arith::mul(uv, arith::Wide<1>::from_u128(dd)), r);
  l.num = r;
  l.den = arith::I128w::from_u128(4 * ww);
  return l;
}

Level level4(const Center& c) {
  // r^2 = |N|^2 / D^2 (N relatif au site d'ancrage, qui est sur la sphere)
  arith::I192 s;
  for (int i = 0; i < 3; ++i) {
    const auto n = arith::I128w::from_i128(c.N[i]);
    arith::I192 sq;
    arith::resize(arith::mul(n, n), sq);
    arith::I192 acc;
    arith::add(s, sq, acc);
    s = acc;
  }
  Level l;
  l.num = s;
  l.den = arith::I128w::from_u128(static_cast<u128>(c.D) * static_cast<u128>(c.D));
  return l;
}

}  // namespace mhgp10::geom
