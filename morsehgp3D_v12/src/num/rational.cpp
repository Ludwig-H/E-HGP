// Operations de num::Rational (tranche S8) : formes reduites par PGCD binaire apres chaque operation, comme
// fractions.Fraction de Python.
#include "num/rational.hpp"

namespace mhgp12::num {

namespace {

// out = n / d reduit (Rational::make divise par le PGCD et rend le denominateur positif).
Outcome reduce(const Big& n, const Big& d, Rational& out) noexcept { return Rational::make(n, d, out); }

}  // namespace

Outcome Rational::make(const Big& numerator, const Big& denominator, Rational& out) noexcept {
  if (denominator.is_zero()) return fail(Reason::arithmetic_invariant);
  Big g, rest, n, d;
  MHGP12_TRY(gcd(numerator, denominator, g));
  n.assign(numerator);
  d.assign(denominator);
  if (!g.is_one()) {
    MHGP12_TRY(divide(n, g, n, rest));
    MHGP12_TRY(divide(d, g, d, rest));
  }
  if (d.negative()) {
    d.negate();
    n.negate();
  }
  out.num_.assign(n);
  out.den_.assign(d);
  return {};
}

Outcome Rational::from_level(const Level& level, Rational& out) noexcept {
  return make(Big::from_wide(to_wide(level.numerator())), Big::from_wide(to_wide(level.denominator())), out);
}

Outcome add(const Rational& a, const Rational& b, Rational& out) noexcept {
  Big left, right, n, d;
  MHGP12_TRY(multiply(a.num_, b.den_, left));
  MHGP12_TRY(multiply(b.num_, a.den_, right));
  MHGP12_TRY(add(left, right, n));
  MHGP12_TRY(multiply(a.den_, b.den_, d));
  return reduce(n, d, out);
}

Outcome subtract(const Rational& a, const Rational& b, Rational& out) noexcept {
  Big left, right, n, d;
  MHGP12_TRY(multiply(a.num_, b.den_, left));
  MHGP12_TRY(multiply(b.num_, a.den_, right));
  MHGP12_TRY(subtract(left, right, n));
  MHGP12_TRY(multiply(a.den_, b.den_, d));
  return reduce(n, d, out);
}

Outcome multiply(const Rational& a, const Rational& b, Rational& out) noexcept {
  Big n, d;
  MHGP12_TRY(multiply(a.num_, b.num_, n));
  MHGP12_TRY(multiply(a.den_, b.den_, d));
  return reduce(n, d, out);
}

Outcome divide(const Rational& a, const Rational& b, Rational& out) noexcept {
  if (b.num_.is_zero()) return fail(Reason::arithmetic_invariant);
  Big n, d;
  MHGP12_TRY(multiply(a.num_, b.den_, n));
  MHGP12_TRY(multiply(a.den_, b.num_, d));
  return reduce(n, d, out);
}

Outcome compare(const Rational& a, const Rational& b, int& order) noexcept {
  Big left, right;
  MHGP12_TRY(multiply(a.numerator(), b.denominator(), left));
  MHGP12_TRY(multiply(b.numerator(), a.denominator(), right));
  order = compare(left, right);
  return {};
}

}  // namespace mhgp12::num
