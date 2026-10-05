// Port des decisions exactes de bench/points_radius.py (sqrt_diff_cmp, sqrt_cmp2, radical_classes, sign_of_radicals)
// sur num::Rational et num::Big ; signature de classe de bench/points_flat.py (class_signature). Chaque decision
// suit la ligne Python qu'elle porte, citee en commentaire.
#include "num/radical.hpp"

namespace mhgp11::num {

namespace {

// SIGNATURE_PRIMES de bench/points_flat.py:44-45.
constexpr std::array<u32, kSignatureCount> kSignaturePrimes{
    2,  3,  5,  7,  11, 13, 17, 19, 23, 29, 31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
    73, 79, 83, 89, 97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173};

u64 power_mod(u64 base, u64 exponent, u64 modulus) noexcept {
  u64 result = 1 % modulus;
  base %= modulus;
  while (exponent != 0) {
    if ((exponent & 1) != 0) result = result * base % modulus;
    base = base * base % modulus;
    exponent >>= 1;
  }
  return result;
}

Outcome set_sign(int value, int& out) noexcept {
  out = value;
  return {};
}

}  // namespace

std::array<u8, kSignatureCount> class_signature(const Big& n) noexcept {
  // class_signature (bench/points_flat.py:58-71) sur N = n d : valuations de meme parite, parties inversibles de
  // meme caractere ; le produit a1 b1 de Python est ici la partie inversible de N.
  std::array<u8, kSignatureCount> out{};
  Big unit;
  for (u32 i = 0; i < kSignatureCount; ++i) {
    const u32 p = kSignaturePrimes[i];
    u32 valuation = 0, rest = 0;
    if (p == 2) {
      valuation = trailing_zeros(n);
      shift_right(n, valuation, unit);
      rest = unit.is_zero() ? 0 : static_cast<u32>(unit.word(0) % 8);
    } else {
      rest = residue(n, p);
      if (rest == 0 && !n.is_zero()) {
        unit.assign(n);
        while (residue(unit, p) == 0) {
          divide_small(unit, p, unit);
          ++valuation;
        }
        rest = residue(unit, p);
      }
    }
    const u32 parity = valuation & 1u;  // un carre parfait a la signature de 1 (defaut b4632db51)
    if (p == 2) {
      out[i] = static_cast<u8>(parity * 8 + rest);
    } else {
      const u64 symbol = power_mod(rest, (p - 1) / 2, p);
      out[i] = static_cast<u8>(parity * 4 + (symbol == 1 ? 1 : 2));
    }
  }
  return out;
}

Result<RadicalSum> RadicalSum::make(MemoryBudget& budget) noexcept {
  RadicalSum out;
  MHGP11_TRY(out.terms_.allocate(kMaxTerms, budget));
  MHGP11_TRY(out.classes_.allocate(kMaxTerms, budget));
  return out;
}

Outcome RadicalSum::add(const Rational& coef, const Rational& radicand) noexcept {
  if (radicand.sign() < 0) return fail(Reason::arithmetic_invariant);
  Big n;
  MHGP11_TRY(multiply(radicand.numerator(), radicand.denominator(), n));
  Rational scaled;
  MHGP11_TRY(divide(coef, Rational::from_big(radicand.denominator()), scaled));
  return add_integer(scaled, n);
}

Outcome RadicalSum::add_integer(const Rational& coef, const Big& radicand) noexcept {
  if (radicand.negative()) return fail(Reason::arithmetic_invariant);
  MHGP11_CHECK(count_ < kMaxTerms && count_ < terms_.size(), radical_sign_budget);
  terms_[count_].coef.assign(coef);
  terms_[count_].radicand.assign(radicand);
  ++count_;
  return {};
}

// radical_classes (bench/points_radius.py:75-90) : chaque terme rejoint la premiere classe dont le rapport des
// radicandes est un carre (square_ratio, :65-72 ; ici N_1 N_2 carre parfait), sinon ouvre une classe ; la
// signature ne fait qu'eviter les tests voues a l'echec. Les classes de coefficient nul sont retirees.
Outcome RadicalSum::group(u32& classes) noexcept {
  classes = 0;
  for (u32 t = 0; t < count_; ++t) {
    const Term& term = terms_[t];
    if (term.radicand.is_zero() || term.coef.is_zero()) continue;  // `if not f: continue`
    const auto signature = class_signature(term.radicand);
    bool joined = false;
    for (u32 c = 0; c < classes && !joined; ++c) {
      Class& cls = classes_[c];
      if (cls.signature != signature) continue;
      Big product, root;
      bool square = false;
      MHGP11_TRY(multiply(term.radicand, cls.rep, product));
      MHGP11_TRY(perfect_square(product, square, root));
      if (!square) continue;
      Rational q, scaled;  // sqrt(N) = (isqrt(N rep) / rep) sqrt(rep)
      MHGP11_TRY(Rational::make(root, cls.rep, q));
      MHGP11_TRY(multiply(term.coef, q, scaled));
      MHGP11_TRY(num::add(cls.coef, scaled, cls.coef));
      joined = true;
    }
    if (joined) continue;
    Class& fresh = classes_[classes];
    fresh.coef.assign(term.coef);
    fresh.rep.assign(term.radicand);
    fresh.signature = signature;
    ++classes;
  }
  u32 kept = 0;  // `[(rep, coef) for rep, coef in classes if coef]`
  for (u32 c = 0; c < classes; ++c) {
    if (classes_[c].coef.is_zero()) continue;
    if (kept != c) {
      classes_[kept].coef.assign(classes_[c].coef);
      classes_[kept].rep.assign(classes_[c].rep);
      classes_[kept].signature = classes_[c].signature;
    }
    ++kept;
  }
  classes = kept;
  return {};
}

// Encadrements de sign_of_radicals (bench/points_radius.py:105-117), sqrt_bounds (:53-58) : sqrt(N) est dans
// [s, s + 1) / 2^b, s = isqrt(N 4^b) ; lo et hi sont multiplies par 2^b > 0, ce qui garde leur signe.
Outcome RadicalSum::refine(u32 classes, int& out) noexcept {
  for (u32 bits = kFirstBits; bits <= kSignBudgetBits; bits *= 2) {
    Rational low, high, term;
    for (u32 c = 0; c < classes; ++c) {
      const Class& cls = classes_[c];
      Big scaled, s, s1;
      MHGP11_TRY(shift_left(cls.rep, 2 * bits, scaled));
      MHGP11_TRY(isqrt(scaled, s));
      MHGP11_TRY(num::add(s, Big::from_u64(1), s1));
      const bool positive = cls.coef.sign() > 0;  // `coef * (a if coef > 0 else b)`
      MHGP11_TRY(multiply(cls.coef, Rational::from_big(positive ? s : s1), term));
      MHGP11_TRY(num::add(low, term, low));
      MHGP11_TRY(multiply(cls.coef, Rational::from_big(positive ? s1 : s), term));
      MHGP11_TRY(num::add(high, term, high));
    }
    trace_.bits = bits;
    if (low.sign() > 0) return set_sign(1, out);
    if (high.sign() < 0) return set_sign(-1, out);
    // 0 est dans [lo, hi] : la somme n'est pas separee a 2^-bits, on double la precision.
  }
  return fail(Reason::radical_sign_budget);  // `raise Refusal`, jamais une egalite supposee
}

// sign_of_radicals (bench/points_radius.py:93-117).
Outcome RadicalSum::sign(int& out) noexcept {
  out = 0;
  trace_ = {};
  u32 classes = 0;
  MHGP11_TRY(group(classes));
  trace_.classes = classes;
  if (classes == 0) return set_sign(0, out);
  if (classes == 1) return set_sign(classes_[0].coef.sign(), out);
  if (classes == 2) {
    const Class& first = classes_[0];
    const Class& second = classes_[1];
    if (first.coef.sign() == second.coef.sign()) return set_sign(first.coef.sign(), out);
    // `sign(c1) * sign(c1 * c1 * r1 - c2 * c2 * r2)`
    Rational left, right, square;
    MHGP11_TRY(multiply(first.coef, first.coef, square));
    MHGP11_TRY(multiply(square, Rational::from_big(first.rep), left));
    MHGP11_TRY(multiply(second.coef, second.coef, square));
    MHGP11_TRY(multiply(square, Rational::from_big(second.rep), right));
    int order = 0;
    MHGP11_TRY(compare(left, right, order));
    return set_sign(first.coef.sign() * order, out);
  }
  return refine(classes, out);
}

// sqrt_diff_cmp (bench/points_radius.py:33-45) ; la recursion finale (d < 0 et u < 0) est un echange unique.
Outcome sqrt_diff_cmp(const Rational& x, const Rational& y, const Rational& u, int& out) noexcept {
  int flip = 1;
  Rational px, py, pu;
  px.assign(x);
  py.assign(y);
  pu.assign(u);
  for (int round = 0; round < 2; ++round) {
    int d = 0;
    MHGP11_TRY(compare(px, py, d));
    const int su = pu.sign();
    if (d >= 0 && su <= 0) return set_sign(flip * (d == 0 && su == 0 ? 0 : 1), out);
    if (d <= 0 && su >= 0) return set_sign(flip * (d == 0 && su == 0 ? 0 : -1), out);
    if (d > 0) {  // u > 0 : sqrt x - sqrt y - u a le signe de (x - y - u^2) - 2 u sqrt y
      Rational w, u2, w2, rhs;
      MHGP11_TRY(subtract(px, py, w));
      MHGP11_TRY(multiply(pu, pu, u2));
      MHGP11_TRY(subtract(w, u2, w));
      if (w.sign() <= 0) return set_sign(flip * (w.is_zero() && py.is_zero() ? 0 : -1), out);
      MHGP11_TRY(multiply(w, w, w2));
      MHGP11_TRY(multiply(u2, py, rhs));
      MHGP11_TRY(multiply(rhs, Rational::from_i64(4), rhs));
      int order = 0;
      MHGP11_TRY(compare(w2, rhs, order));
      return set_sign(flip * order, out);
    }
    // d < 0 et u < 0 : `return -sqrt_diff_cmp(y, x, -u)`
    Rational swap;
    swap.assign(px);
    px.assign(py);
    py.assign(swap);
    pu.negate();
    flip = -flip;
  }
  return fail(Reason::arithmetic_invariant);
}

Outcome sqrt_cmp2(const Rational& a, const Rational& b, const Rational& c, const Rational& d, int& out) noexcept {
  // `sqrt_diff_cmp(a * b, c * d, ((c + d) - (a + b)) / 2)`
  Rational ab, cd, left, right, half;
  MHGP11_TRY(multiply(a, b, ab));
  MHGP11_TRY(multiply(c, d, cd));
  MHGP11_TRY(num::add(c, d, left));
  MHGP11_TRY(num::add(a, b, right));
  MHGP11_TRY(subtract(left, right, half));
  Rational two = Rational::from_i64(2);
  MHGP11_TRY(divide(half, two, half));
  return sqrt_diff_cmp(ab, cd, half, out);
}

Outcome compare_dates(const std::array<Rational, 3>& first, const std::array<Rational, 3>& second, RadicalSum& sum,
                      int& out) noexcept {
  // `[(1, t), (1, m), (-1, q), (-1, t'), (-1, m'), (1, q')]`
  const Rational plus = Rational::from_i64(1), minus = Rational::from_i64(-1);
  sum.clear();
  MHGP11_TRY(sum.add(plus, first[0]));
  MHGP11_TRY(sum.add(plus, first[1]));
  MHGP11_TRY(sum.add(minus, first[2]));
  MHGP11_TRY(sum.add(minus, second[0]));
  MHGP11_TRY(sum.add(minus, second[1]));
  MHGP11_TRY(sum.add(plus, second[2]));
  return sum.sign(out);
}

}  // namespace mhgp11::num
