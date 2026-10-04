// Comptes exacts du lemme G (counts.hpp) : domaine d'une forme de boule et comptes d'une boule depuis sa fermeture.
#include "supports/supports.hpp"

namespace mhgp11::supports {

Result<Shape> make_shape(u32 p, u32 m, u32 q, Order k) noexcept {
  MHGP11_TRY(check_shell(m));
  if (k < 1 || k > kMaxOrder || q < 2 || q > 4 || m < q || p > kMaxInterior || p + q > u32{k} + 1)
    return fail(Reason::supports_invariant);
  Shape shape;
  shape.p_ = p;
  shape.m_ = m;
  shape.q_ = q;
  shape.k_ = k;
  return shape;
}

Result<Shape> ball_shape(const FullDomain& domain, BallIdx ball, Order k) noexcept {
  const auto& cat = domain.catalogue();
  if (idx(ball) >= cat.balls()) return fail(Reason::parameter_out_of_range);
  const CatalogueBall& data = cat.balls_data()[idx(ball)];
  return make_shape(data.p, data.m, data.qmin, k);
}

Result<BallCounts> ball_counts(const Shape& shape, const Closure& closure) noexcept {
  const i32 p = static_cast<i32>(shape.interior()), m = static_cast<i32>(shape.shell());
  const i32 q = static_cast<i32>(shape.qmin()), k = i32{shape.order()}, t = static_cast<i32>(shape.t());
  // Fermeture d'une coquille de cette forme : sinon un compte pourrait deborder ou mentir. m <= 24 : C(m, j) en u32.
  if (closure.shell() != shape.shell() || closure.parts(shape.qmin()) == 0 || closure.parts(shape.shell()) != 1)
    return fail(Reason::supports_invariant);
  for (i32 j = 0; j <= m; ++j) {
    const u32 parts = closure.parts(static_cast<u32>(j));
    if (parts > supports_detail::binomial(m, j) || (j < q && parts != 0)) return fail(Reason::supports_invariant);
  }
  BallCounts counts;
  counts.kparties_reliees = supports_detail::binomial(p + m, k);
  counts.compressed_parts = supports_detail::binomial(m, t);  // 0 hors fenetre (t > m), comme N_t
  counts.strict_traces = counts.compressed_parts - closure.parts(static_cast<u32>(t));
  u64 cofaces = 0;  // somme_j C(p, K+1-j) N_j <= C(p+m, K+1) < 2^32 (Vandermonde) ; garde exacte malgre tout
  for (i32 j = q; j <= m; ++j)
    cofaces += u64{supports_detail::binomial(p, k + 1 - j)} * closure.parts(static_cast<u32>(j));
  if (cofaces > 0xFFFFFFFFull) return fail(Reason::supports_invariant);
  counts.cofaces = static_cast<u32>(cofaces);
  counts.gabriel_cofaces = closure.parts(static_cast<u32>(t) + 1);  // 0 si t + 1 > m
  return counts;
}

}  // namespace mhgp11::supports
