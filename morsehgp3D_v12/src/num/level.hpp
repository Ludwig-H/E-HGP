// Niveau rationnel exact non negatif, non reduit, denominateur strictement positif.
// Port des produits croises de R2 geometry.cpp ; aucun affichage double ni filtre approche dans cette tranche.
// Stockage au budget du profil (num/budgets.hpp) ; la comparaison choisit sa largeur par les longueurs reelles des deux
// produits croises : i128 natif si les deux tiennent en 127 bits, sinon 256, 384 ou 512 bits, les largeurs des trois
// paliers pour 14s+20 (docs/CONTRAT_NUMERIQUE.md, paragraphe 3 : 512 bits suffisent a s = 33).
#pragma once

#include "num/budgets.hpp"

namespace mhgp12::num {

class Level {
 public:
  using Numerator = Int<DomainBudget::level_numerator>;
  using Denominator = Int<DomainBudget::level_denominator>;
  Level() noexcept = default;

  template <int N, int D>
  static Result<Level> make(const Wide<N>& numerator, const Wide<D>& denominator) noexcept {
    const auto n = narrow<DomainBudget::level_numerator>(numerator);
    const auto d = narrow<DomainBudget::level_denominator>(denominator);
    if (numerator.sign() < 0 || denominator.sign() <= 0 || !n || !d)
      return fail(Reason::parameter_out_of_range);
    return Level(*n, *d);
  }
  const Numerator& numerator() const noexcept { return numerator_; }
  const Denominator& denominator() const noexcept { return denominator_; }

 private:
  Level(Numerator n, Denominator d) noexcept : numerator_(n), denominator_(d) {}
  Numerator numerator_{};
  Denominator denominator_ = integer_one<DomainBudget::level_denominator>();
};

namespace detail {
// Ordre de deux valeurs deja calculees : seule decision des voies de compare(Level, Level).
template <class T>
constexpr int three_way(const T& left, const T& right) noexcept {
  if constexpr (std::is_same_v<T, i128>) return (left > right) - (left < right);
  else return compare(left, right);
}
// Produits croises dans Words mots, la largeur d'un palier : les longueurs lues par l'appelant majorent celles des
// produits, multiply_into ne refuse donc pas ; un refus serait un invariant viole, rendu comme egalite impossible a
// confondre ici (l'appelant a deja controle les longueurs).
template <int Words>
int level_order(const Level& a, const Level& b) noexcept {
  Wide<Words> left, right;
  const bool fits = multiply_into(to_wide(a.numerator()), to_wide(b.denominator()), left) &&
                    multiply_into(to_wide(b.numerator()), to_wide(a.denominator()), right);
  return fits ? three_way(left, right) : 0;
}
}  // namespace detail

inline int compare(const Level& a, const Level& b, LaneCount* lanes = nullptr) noexcept {
  const int left_bits = to_wide(a.numerator()).bit_length() + to_wide(b.denominator()).bit_length();
  const int right_bits = to_wide(b.numerator()).bit_length() + to_wide(a.denominator()).bit_length();
  const int widest = left_bits > right_bits ? left_bits : right_bits;
  if (widest <= 127) {
    // Chaque facteur a moins de 127 bits : narrow ne refuse pas ; chaque produit < 2^127.
    count_lane(lanes, Lane::native);
    const i128 left = *narrow<127>(to_wide(a.numerator())) * *narrow<127>(to_wide(b.denominator()));
    const i128 right = *narrow<127>(to_wide(b.numerator())) * *narrow<127>(to_wide(a.denominator()));
    return detail::three_way(left, right);
  }
  count_lane(lanes, Lane::wide);
  // Chaque facteur a au plus `widest` bits et la somme des deux longueurs majore celle du produit.
  if (widest <= 256) return detail::level_order<4>(a, b);
  if (widest <= 384) return detail::level_order<6>(a, b);
  static_assert(DomainBudget::level_numerator + DomainBudget::level_denominator <= 512,
                "num : produits croises du profil dans 512 bits");
  return detail::level_order<8>(a, b);
}

}  // namespace mhgp12::num
