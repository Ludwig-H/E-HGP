// Vue interne d'un centre a+N/D pour les predicats : construite seulement depuis les trois proprietaires (Sphere,
// Q3Candidate, Q4Candidate), jamais depuis des coefficients nus. Elle porte l'etendue du support, les deux domaines de
// certificat et, quand ils y tiennent, les coefficients en i128 (toujours aux paliers etroit et moyen, et sous tout
// certificat ; au profil 21 ou 24 le stockage est deja i128, sans copie).
#pragma once

#include <algorithm>

#include "num/geometry_internal.hpp"

namespace mhgp12::num::detail {

class CenterView {
 public:
  explicit CenterView(const Sphere& sphere) noexcept
      : CenterView(sphere.anchor(), sphere.numerator(), sphere.denominator(), sphere.presentation_arity(),
                   sphere.support_span(), sphere.power_domain(), sphere.orientation_domain()) {}
  explicit CenterView(const Q3Candidate& sphere) noexcept
      : CenterView(sphere.anchor(), sphere.numerator(), sphere.denominator(), sphere.presentation_arity(),
                   sphere.support_span(), sphere.power_domain(), sphere.orientation_domain()) {}
  explicit CenterView(const Q4Candidate& sphere) noexcept
      : CenterView(sphere.anchor(), sphere.numerator(), sphere.denominator(), sphere.presentation_arity(),
                   sphere.support_span(), sphere.power_domain(), sphere.orientation_domain()) {}
  CenterView(const CenterView&) = delete;  // n128_ peut designer la copie interne : jamais recopiee
  CenterView& operator=(const CenterView&) = delete;
  Point anchor() const noexcept { return anchor_; }
  const std::array<CenterInt, 3>& numerator() const noexcept { return *numerator_; }
  const CenterDen& denominator() const noexcept { return denominator_; }
  u8 presentation_arity() const noexcept { return arity_; }
  int span() const noexcept { return span_; }
  int power_domain() const noexcept { return power_domain_; }
  int orientation_domain() const noexcept { return orientation_domain_; }
  // Vrai si N et D tiennent en i128 ; alors n128() et d128() sont exacts.
  bool native_coefficients() const noexcept { return native_; }
  const std::array<i128, 3>& n128() const noexcept { return *n128_; }
  i128 d128() const noexcept { return d128_; }
  // Signe de N_j, quel que soit le stockage.
  int numerator_sign(int j) const noexcept { return to_wide((*numerator_)[j]).sign(); }

 private:
  CenterView(Point anchor, const std::array<CenterInt, 3>& numerator, CenterDen denominator, u8 arity, int span,
             int power, int orientation) noexcept
      : anchor_(anchor), numerator_(&numerator), denominator_(denominator), arity_(arity), span_(span),
        power_domain_(power), orientation_domain_(orientation) {
    const auto d = as_i128(denominator);
    native_ = d.has_value();
    if (native_) d128_ = *d;
    n128_ = direct(numerator);
    if (n128_ == nullptr) {  // stockage large (profil 32) : copie controlee, si elle tient
      for (int j = 0; j < 3 && native_; ++j) {
        const auto value = as_i128(numerator[j]);
        native_ = value.has_value();
        if (native_) copy_[j] = *value;
      }
      n128_ = &copy_;
    }
  }
  // Au profil 21 ou 24 le stockage est deja i128 : aucune copie.
  template <class T>
  static const std::array<i128, 3>* direct(const std::array<T, 3>& numerator) noexcept {
    if constexpr (std::is_same_v<T, i128>) return &numerator;
    else return nullptr;
  }
  Point anchor_;
  const std::array<CenterInt, 3>* numerator_;  // le proprietaire survit a la vue
  CenterDen denominator_;  // rendu par valeur par les proprietaires
  u8 arity_;
  int span_;
  int power_domain_, orientation_domain_;
  bool native_ = false;
  const std::array<i128, 3>* n128_ = nullptr;
  std::array<i128, 3> copy_{};
  i128 d128_ = 0;
};

// Voie de la puissance D|v|^2 - 2N.v pour une requete d'etendue t autour de l'ancre, dans le repere support+requete
// d'etendue u = max(s, t) : q1 et q2 natifs a toute etendue (< 12 M^2, 2u+4 <= 70 bits) ; q4 natif jusqu'au palier
// moyen (normales a ancrage commun, < 72 M^5, 5u+7 <= 127) ; q3 natif au palier etroit (< 216 M^6, 6u+8 <= 104) ;
// sinon certificat de domaine t, sinon essai controle (coefficients en i128), sinon voie large.
inline Lane power_lane(const CenterView& center, int t) noexcept {
  const Tier tier = tier_of(std::max(center.span(), t));
  const u8 q = center.presentation_arity();
  static_assert(TierBudgets<Tier::wide>::dot + 2 <= 127 && TierBudgets<Tier::medium>::side4 <= 127 &&
                TierBudgets<Tier::narrow>::side <= 127);
  if (q <= 2 || (q == 4 && tier != Tier::wide) || (q == 3 && tier == Tier::narrow)) return Lane::native;
  if (!center.native_coefficients()) return Lane::wide;
  if (center.power_domain() >= t) return Lane::certified;
  return Lane::checked;
}

// Mots de la voie large de la puissance : budget 6u+8 au plafond du palier de u = max(s, t).
inline int power_words(const CenterView& center, int t) noexcept {
  const Tier tier = tier_of(std::max(center.span(), t));
  return tier == Tier::narrow ? words_for(TierBudgets<Tier::narrow>::side)
       : tier == Tier::medium ? words_for(TierBudgets<Tier::medium>::side) : words_for(TierBudgets<Tier::wide>::side);
}

}  // namespace mhgp12::num::detail
