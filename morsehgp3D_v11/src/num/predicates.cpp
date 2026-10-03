// Predicats R2 portes avec budgets par expression. Pas de conversion flottante, pas de filtre heuristique.
#include "num/geometry_internal.hpp"
#include "num/power_checked.hpp"

namespace mhgp11::num {
namespace {

// Vue synchrone privee a cette unite : seuls les deux proprietaires certifies peuvent la construire.
// Aucun appel public n'accepte un tuple de coefficients ou un type satisfaisant seulement des getters.
class CenterView {
 public:
  explicit CenterView(const Sphere& sphere) noexcept
      : anchor_(sphere.anchor()), numerator_(sphere.numerator()), denominator_(sphere.denominator()),
        arity_(sphere.presentation_arity()) {}
  explicit CenterView(const Q4Candidate& sphere) noexcept
      : anchor_(sphere.anchor()), numerator_(sphere.numerator()), denominator_(sphere.denominator()),
        arity_(sphere.presentation_arity()) {}
  Point anchor() const noexcept { return anchor_; }
  const std::array<CenterInt, 3>& numerator() const noexcept { return numerator_; }
  CenterDen denominator() const noexcept { return denominator_; }
  u8 presentation_arity() const noexcept { return arity_; }

 private:
  Point anchor_;
  const std::array<CenterInt, 3>& numerator_;
  CenterDen denominator_;
  u8 arity_;
};

bool use_native_power(const CenterView& sphere) noexcept {
  return Budget::side <= 127 || sphere.presentation_arity() != 3;
}

// Precondition interne : use_native_power(sphere). M=2^B, |v_j|<M ; chaque carre et somme de dot<3M^2
// tient en i64. La conversion en i128 est exacte ; |-2*v_j|<2M, avant multiplication par N_j.
// q1 : somme absolue <3M^2. q2 : premier terme <6M^2, chacun des trois suivants <2M^2, total <12M^2.
// q4 : cross(b-a,c-a)_j est le determinant de trois points du MEME carre [0,M-1]^2. Multiaffine,
// son maximum absolu est aux coins, ou il vaut 0 ou (M-1)^2 : donc <M^2, pas pour deux Vec arbitraires.
// Cramer donne D<6M^3 et |N_j|<9M^4 : chacun des quatre termes <18M^5, somme des magnitudes <72M^5.
// q3 : uniquement si Budget::side<=127 ; D<24M^4, |N_j|<24M^5, somme des magnitudes <216M^6.
// Ces sommes majorent CHAQUE produit et somme partielle, sans utiliser une annulation ni la convexite.
i128 native_power(const CenterView& sphere, Point point) noexcept {
  static_assert(Budget::dot <= 63 && 2 * kCoordBits + 4 <= 127 && 5 * kCoordBits + 7 <= 127);
  static_assert(Budget::side == 6 * kCoordBits + 8 && 5 * kCoordBits + 7 <= Budget::side);
  const auto v = detail::difference(point, sphere.anchor());
  i128 total = sphere.denominator() * i128{detail::dot(v, v)};
  for (int j = 0; j < 3; ++j) total += sphere.numerator()[j] * (-2 * i128{v[j]});
  return total;
}

Result<SideInt> wide_power(const CenterView& sphere, Point point) noexcept {
  constexpr int words = (Budget::side + 63) / 64;
  const auto v = detail::difference(point, sphere.anchor());
  auto first = detail::product<words>(sphere.denominator(), detail::dot(v, v));
  if (!first.ok()) return first.outcome();
  auto total = first.value();
  for (int j = 0; j < 3; ++j) {
    auto term = detail::product<words>(sphere.numerator()[j], -2 * i128{v[j]});
    if (!term.ok()) return term.outcome();
    auto sum = detail::require_add(total, term.value());
    if (!sum.ok()) return sum.outcome();
    total = sum.value();
  }
  return detail::require_fit<Budget::side>(total);
}

std::optional<i128> checked_power(const CenterView& sphere, Point point) noexcept {
  const auto v = detail::difference(point, sphere.anchor());
  // Ces operations PRECEDENT les builtins : |v_j|<2^B, norme<3*2^(2B)<2^50 et facteurs<2^(B+1).
  static_assert(2 * kCoordBits + 2 <= 50 && kCoordBits + 1 < 63);
  const std::array<i64, 3> factors{-2 * v[0], -2 * v[1], -2 * v[2]};
  return detail::checked_power_sum(sphere.denominator(), sphere.numerator(), detail::dot(v, v), factors);
}

Result<SideInt> center_power(const CenterView& sphere, Point point) noexcept {
  if (use_native_power(sphere))
    return detail::require_fit<Budget::side>(to_wide(native_power(sphere, point)));
  if (const auto value = checked_power(sphere, point))
    return detail::require_fit<Budget::side>(to_wide(*value));
  return wide_power(sphere, point);
}

Result<int> center_side(const CenterView& sphere, Point point) noexcept {
  if (use_native_power(sphere)) return detail::sign(native_power(sphere, point));
  if (const auto value = checked_power(sphere, point)) return detail::sign(*value);
  auto value = wide_power(sphere, point);
  if (!value.ok()) return value.outcome();
  return to_wide(value.value()).sign();
}

struct BoundTerms {
  i64 norm_lower = 0, norm_upper = 0;
  std::array<i64, 3> linear_lower{}, linear_upper{};
};

BoundTerms bound_terms(const CenterView& sphere, const Box& box) noexcept {
  const auto lo = detail::difference(box.lo(), sphere.anchor());
  const auto hi = detail::difference(box.hi(), sphere.anchor());
  BoundTerms terms;
  // |lo_j|,|hi_j|<M : les carres <M^2, leurs trois sommes <3M^2, les facteurs doubles <2M.
  static_assert(Budget::dot <= 63 && kCoordBits + 1 <= 63);
  for (int j = 0; j < 3; ++j) {
    const i64 lo2 = lo[j] * lo[j], hi2 = hi[j] * hi[j];
    terms.norm_lower += lo[j] > 0 ? lo2 : hi[j] < 0 ? hi2 : 0;
    terms.norm_upper += lo2 > hi2 ? lo2 : hi2;
    const bool nonnegative = sphere.numerator()[j] >= 0;
    terms.linear_lower[j] = -2 * (nonnegative ? hi[j] : lo[j]);
    terms.linear_upper[j] = -2 * (nonnegative ? lo[j] : hi[j]);
  }
  return terms;
}

template <int Words>
Result<PowerBounds> checked_bounds(const Wide<Words>& lower, const Wide<Words>& upper) noexcept {
  if (compare(lower, upper) > 0) return fail(Reason::arithmetic_invariant);
  const auto lo = detail::require_fit<Budget::side>(lower), hi = detail::require_fit<Budget::side>(upper);
  if (!lo.ok()) return lo.outcome();
  if (!hi.ok()) return hi.outcome();
  return PowerBounds{lo.value(), hi.value()};
}

Result<PowerBounds> center_power_bounds(const CenterView& sphere, const Box& box) noexcept {
  const auto terms = bound_terms(sphere, box);
  // D>0. Separer les extrema peut elargir l'intervalle, jamais l'inverser ou supprimer un contact.
  // Les quatre termes de CHAQUE borne ont les memes majorants absolus que native_power : <12M^2 pour q2,
  // <72M^5 pour q4 grace aux cross a ancrage commun, <216M^6 pour q3. Toute somme partielle est bornee ainsi.
  static_assert(2 * kCoordBits + 4 <= 127 && 5 * kCoordBits + 7 <= 127);
  static_assert(Budget::side == 6 * kCoordBits + 8 && 5 * kCoordBits + 7 <= Budget::side);
  if (use_native_power(sphere)) {
    i128 lower = sphere.denominator() * i128{terms.norm_lower};
    i128 upper = sphere.denominator() * i128{terms.norm_upper};
    for (int j = 0; j < 3; ++j) {
      lower += sphere.numerator()[j] * terms.linear_lower[j];
      upper += sphere.numerator()[j] * terms.linear_upper[j];
    }
    return checked_bounds(to_wide(lower), to_wide(upper));
  }
  const auto native_lower = detail::checked_power_sum(sphere.denominator(), sphere.numerator(),
                                                      terms.norm_lower, terms.linear_lower);
  const auto native_upper = detail::checked_power_sum(sphere.denominator(), sphere.numerator(),
                                                      terms.norm_upper, terms.linear_upper);
  // Aucun resultat partiel : si l'un des essais refuse, reprendre LES DEUX bornes dans la voie Wide historique.
  if (native_lower && native_upper) return checked_bounds(to_wide(*native_lower), to_wide(*native_upper));
  constexpr int words = (Budget::side + 63) / 64;
  auto lower = detail::product<words>(sphere.denominator(), terms.norm_lower);
  auto upper = detail::product<words>(sphere.denominator(), terms.norm_upper);
  if (!lower.ok()) return lower.outcome();
  if (!upper.ok()) return upper.outcome();
  for (int j = 0; j < 3; ++j) {
    const auto lo = detail::product<words>(sphere.numerator()[j], terms.linear_lower[j]);
    const auto hi = detail::product<words>(sphere.numerator()[j], terms.linear_upper[j]);
    if (!lo.ok()) return lo.outcome();
    if (!hi.ok()) return hi.outcome();
    lower = detail::require_add(lower.value(), lo.value());
    upper = detail::require_add(upper.value(), hi.value());
    if (!lower.ok()) return lower.outcome();
    if (!upper.ok()) return upper.outcome();
  }
  return checked_bounds(lower.value(), upper.value());
}

Result<int> center_orientation(Point a, Point b, Point c, const CenterView& center) noexcept {
  constexpr int words = (Budget::center_orientation + 63) / 64;
  const auto normal = detail::cross(detail::difference(b, a), detail::difference(c, a));
  const auto offset = detail::difference(center.anchor(), a);
  Wide<words> total;
  for (int j = 0; j < 3; ++j) {
    // N + D*(anchor-a) < 48 M^5, donc < 2^(5B+6) <= 2^126 : i128 reste exact.
    static_assert(5 * kCoordBits + 6 <= 127);
    const i128 coordinate = center.numerator()[j] + center.denominator() * offset[j];
    auto term = detail::product<words>(coordinate, normal[j]);
    if (!term.ok()) return term.outcome();
    auto sum = detail::require_add(total, term.value());
    if (!sum.ok()) return sum.outcome();
    total = sum.value();
  }
  if (total.bit_length() > Budget::center_orientation) return fail(Reason::arithmetic_invariant);
  return total.sign();
}

Result<bool> center_inside(const CenterView& center, Point a, Point b, Point c, Point d) noexcept {
  const std::array<Point, 4> points{a, b, c, d};
  for (int opposite = 0; opposite < 4; ++opposite) {
    std::array<Point, 3> face{};
    int j = 0;
    for (int i = 0; i < 4; ++i)
      if (i != opposite) face[j++] = points[i];
    const int vertex_sign = detail::sign(orientation(face[0], face[1], face[2], points[opposite]));
    if (vertex_sign == 0) return false;
    auto center_sign = center_orientation(face[0], face[1], face[2], center);
    if (!center_sign.ok()) return center_sign.outcome();
    if (center_sign.value() != vertex_sign) return false;
  }
  return true;
}

bool center_midpoint(const CenterView& center, Point a, Point b) noexcept {
  // Chaque cote < 96 M^5 < 2^(5B+7) <= 2^127 ; les intermediaires signes restent representables.
  static_assert(5 * kCoordBits + 7 <= 127);
  const auto anchor = center.anchor().coordinates(), ac = a.coordinates(), bc = b.coordinates();
  for (int j = 0; j < 3; ++j)
    if (2 * (center.denominator() * anchor[j] + center.numerator()[j]) !=
        center.denominator() * (i128{ac[j]} + bc[j])) return false;
  return true;
}

}  // namespace

DotInt squared_distance(Point a, Point b) noexcept {
  // Elargir en SIGNE avant chaque soustraction : les u32 de Point ne doivent jamais soustraire en non signe.
  // |delta|<2^B ; chaque carre et chaque somme partielle positive <=3*(2^B-1)^2<2^50 pour B<=24.
  static_assert(std::same_as<DotInt, i64> && 2 * kCoordBits + 2 <= 50);
  const i64 dx = i64{a.x()} - b.x(), dy = i64{a.y()} - b.y(), dz = i64{a.z()} - b.z();
  return dx * dx + dy * dy + dz * dz;
}

Result<SideInt> power(const Sphere& sphere, Point point) noexcept {
  return center_power(CenterView(sphere), point);
}
Result<SideInt> power(const Q4Candidate& sphere, Point point) noexcept {
  return center_power(CenterView(sphere), point);
}
Result<int> side(const Sphere& sphere, Point point) noexcept {
  return center_side(CenterView(sphere), point);
}
Result<int> side(const Q4Candidate& sphere, Point point) noexcept {
  return center_side(CenterView(sphere), point);
}
Result<PowerBounds> power_bounds(const Sphere& sphere, const Box& box) noexcept {
  return center_power_bounds(CenterView(sphere), box);
}

DeterminantInt orientation(Point a, Point b, Point c, Point d) noexcept {
  const auto u = detail::difference(b, a), v = detail::difference(c, a), w = detail::difference(d, a);
  const auto normal = detail::cross(u, v);
  static_assert(Budget::determinant <= 127);
  // La somme a six monomes est < 6 M^3 < 2^Budget::determinant ; la conversion en i64 du profil 18 est exacte.
  return static_cast<DeterminantInt>(i128{normal[0]} * w[0] + i128{normal[1]} * w[1] + i128{normal[2]} * w[2]);
}

Result<int> orientation(Point a, Point b, Point c, const Sphere& center) noexcept {
  return center_orientation(a, b, c, CenterView(center));
}
Result<int> orientation(Point a, Point b, Point c, const Q4Candidate& center) noexcept {
  return center_orientation(a, b, c, CenterView(center));
}

bool strictly_acute(Point a, Point b, Point c) noexcept {
  return detail::dot(detail::difference(b, a), detail::difference(c, a)) > 0 &&
         detail::dot(detail::difference(a, b), detail::difference(c, b)) > 0 &&
         detail::dot(detail::difference(a, c), detail::difference(b, c)) > 0;
}

Result<bool> strictly_inside(const Sphere& center, Point a, Point b, Point c, Point d) noexcept {
  return center_inside(CenterView(center), a, b, c, d);
}
Result<bool> strictly_inside(const Q4Candidate& center, Point a, Point b, Point c, Point d) noexcept {
  return center_inside(CenterView(center), a, b, c, d);
}
bool is_midpoint(const Sphere& center, Point a, Point b) noexcept {
  return center_midpoint(CenterView(center), a, b);
}
bool is_midpoint(const Q4Candidate& center, Point a, Point b) noexcept {
  return center_midpoint(CenterView(center), a, b);
}

}  // namespace mhgp11::num
