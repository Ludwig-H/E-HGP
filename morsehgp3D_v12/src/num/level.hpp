// Niveau rationnel exact non negatif, non reduit, denominateur strictement positif.
// Port des produits croises de R2 geometry.cpp ; aucun affichage double ni filtre approche dans cette tranche.
#pragma once

#include "num/budgets.hpp"

namespace mhgp12::num {

class Level {
 public:
  using Numerator = Int<Budget::level_numerator>;
  using Denominator = Int<Budget::level_denominator>;
  Level() noexcept = default;

  template <int N, int D>
  static Result<Level> make(const Wide<N>& numerator, const Wide<D>& denominator) noexcept {
    const auto n = narrow<Budget::level_numerator>(numerator);
    const auto d = narrow<Budget::level_denominator>(denominator);
    if (numerator.sign() < 0 || denominator.sign() <= 0 || !n || !d)
      return fail(Reason::parameter_out_of_range);
    return Level(*n, *d);
  }
  const Numerator& numerator() const noexcept { return numerator_; }
  const Denominator& denominator() const noexcept { return denominator_; }

 private:
  Level(Numerator n, Denominator d) noexcept : numerator_(n), denominator_(d) {}
  Numerator numerator_{};
  Denominator denominator_ = integer_one<Budget::level_denominator>();
};

inline int compare(const Level& a, const Level& b) noexcept {
  const auto left = multiply(to_wide(a.numerator()), to_wide(b.denominator()));
  const auto right = multiply(to_wide(b.numerator()), to_wide(a.denominator()));
  return compare(left, right);
}

}  // namespace mhgp12::num
