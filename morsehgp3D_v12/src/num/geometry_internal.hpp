// Outils internes des formules geometriques : differences certifiees par Point, produits natifs bornes par l'etendue,
// conversions entre le stockage du profil et les voies de calcul, entier exact de la voie large des constructions.
#pragma once

#include "num/geometry.hpp"

namespace mhgp12::num::detail {

using Vec = std::array<i64, 3>;
inline Vec difference(Point a, Point b) noexcept {
  return {i64{a.x()} - b.x(), i64{a.y()} - b.y(), i64{a.z()} - b.z()};
}
// Etendue en bits d'un vecteur de differences : le plus petit t tel que |v_j| < 2^t pour chaque axe.
inline int span_of(const Vec& v) noexcept {
  u64 widest = 0;
  for (const i64 value : v) {
    const u64 magnitude = value < 0 ? u64{0} - static_cast<u64>(value) : static_cast<u64>(value);
    widest = magnitude > widest ? magnitude : widest;
  }
  return span_bits(widest);
}
// Etendue des points de presentation d'un support (NUM-REPERE) : 0 a B.
template <class... P>
inline u8 presentation_span(Point first, P... rest) noexcept {
  Frame frame;
  frame.add_site(first.x(), first.y(), first.z());
  (frame.add_site(rest.x(), rest.y(), rest.z()), ...);
  return static_cast<u8>(frame.span());
}
// Precondition : etendue des deux vecteurs au plus 30 (paliers etroit et moyen) ; somme < 3 M^2 < 2^63.
inline i64 dot(const Vec& a, const Vec& b) noexcept {
  static_assert(SpanBudgets<30>::dot <= 63);
  return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}
// Toute etendue jusqu'a 33 : somme < 3*2^66 < 2^127.
inline i128 dot128(const Vec& a, const Vec& b) noexcept {
  static_assert(SpanBudgets<kMaxSpan>::dot <= 127);
  return i128{a[0]} * b[0] + i128{a[1]} * b[1] + i128{a[2]} * b[2];
}
// Precondition : etendue au plus 31 ; difference de deux produits < 2 M^2 <= 2^63.
inline Vec cross(const Vec& a, const Vec& b) noexcept {
  static_assert(SpanBudgets<31>::cross <= 63);
  return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]};
}
using Vec128 = std::array<i128, 3>;
inline Vec128 cross128(const Vec& a, const Vec& b) noexcept {
  return {i128{a[1]} * b[2] - i128{a[2]} * b[1], i128{a[2]} * b[0] - i128{a[0]} * b[2],
          i128{a[0]} * b[1] - i128{a[1]} * b[0]};
}
inline int sign(i128 value) noexcept { return (value > 0) - (value < 0); }

template <int N, int D>
Result<Level> checked_level(const Wide<N>& numerator, const Wide<D>& denominator) noexcept {
  auto out = Level::make(numerator, denominator);
  if (!out.ok()) return fail(Reason::arithmetic_invariant);
  return out.value();
}

// Produit exact a Words mots de deux entiers (i64, i128 ou Wide) issus des bornes de Point/Sphere, jamais d'entiers
// utilisateur nus. multiply_into ne parcourt que les mots utiles ; un produit qui ne tiendrait pas est un invariant.
template <int Words, class A, class B>
Result<Wide<Words>> product(const A& a, const B& b) noexcept {
  Wide<Words> out;
  if (!multiply_into(to_wide(a), to_wide(b), out)) return fail(Reason::arithmetic_invariant);
  return out;
}

// Stockage du profil -> voie i128 : rien si la valeur ne tient pas en 127 bits (conversion controlee, jamais une
// troncature). Voie -> stockage : refus d'invariant si la valeur sort du budget de stockage.
template <class T>
std::optional<i128> as_i128(const T& value) noexcept {
  if constexpr (std::is_same_v<T, i128> || std::is_same_v<T, i64>) return i128{value};
  else return narrow<127>(value);
}
template <class T>
Result<CenterInt> store_numerator(const T& value) noexcept {
  return require_fit<DomainBudget::center_numerator>(to_wide(value));
}
template <class T>
Result<CenterDen> store_denominator(const T& value) noexcept {
  return require_fit<DomainBudget::center_denominator>(to_wide(value));
}

// Entier exact signe de 320 bits pour la voie large des constructions (palier large, etendues 25 a 32) : chaque
// operation est controlee et un depassement leve un drapeau collant, lu une fois en fin de formule (invariant viole,
// jamais une valeur tronquee). 320 bits couvrent le plus large intermediaire du profil 32 (niveau q4, 8s+12 = 268).
struct Exact {
  Wide<5> value{};
  bool overflow = false;
  Exact() noexcept = default;
  explicit Exact(i128 v) noexcept { overflow = !resize(to_wide(v), value); }
  template <int K>
  explicit Exact(const Wide<K>& v) noexcept { overflow = !resize(v, value); }
  friend Exact operator+(const Exact& a, const Exact& b) noexcept {
    Exact out;
    out.overflow = a.overflow || b.overflow || !add(a.value, b.value, out.value);
    return out;
  }
  friend Exact operator-(const Exact& a, const Exact& b) noexcept {
    Exact out;
    out.overflow = a.overflow || b.overflow || !subtract(a.value, b.value, out.value);
    return out;
  }
  friend Exact operator*(const Exact& a, const Exact& b) noexcept {
    Exact out;
    out.overflow = a.overflow || b.overflow || !multiply_into(a.value, b.value, out.value);
    return out;
  }
  int sign() const noexcept { return value.sign(); }
};

}  // namespace mhgp12::num::detail
