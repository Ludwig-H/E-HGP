// Predicats R2 portes avec budgets par expression. Pas de conversion flottante, pas de filtre heuristique.
#include "num/geometry_internal.hpp"
#include "num/lattice_bounds.hpp"
#include "num/power_checked.hpp"

#include <algorithm>

namespace mhgp11::num {
namespace {

// Vue synchrone privee a cette unite : seuls les trois proprietaires certifies peuvent la construire.
// Aucun appel public n'accepte un tuple de coefficients ou un type satisfaisant seulement des getters.
class CenterView {
 public:
  explicit CenterView(const Sphere& sphere) noexcept
      : anchor_(sphere.anchor()), numerator_(sphere.numerator()), denominator_(sphere.denominator()),
        arity_(sphere.presentation_arity()), q3_power_i128_(sphere.q3_power_i128_certified()),
        orientation_i128_(sphere.orientation_i128_certified()) {}
  explicit CenterView(const Q3Candidate& sphere) noexcept
      : anchor_(sphere.anchor()), numerator_(sphere.numerator()), denominator_(sphere.denominator()),
        arity_(sphere.presentation_arity()), q3_power_i128_(sphere.q3_power_i128_certified()),
        orientation_i128_(sphere.orientation_i128_certified()) {}
  explicit CenterView(const Q4Candidate& sphere) noexcept
      : anchor_(sphere.anchor()), numerator_(sphere.numerator()), denominator_(sphere.denominator()),
        arity_(sphere.presentation_arity()), q3_power_i128_(false),
        orientation_i128_(sphere.orientation_i128_certified()) {}
  Point anchor() const noexcept { return anchor_; }
  const std::array<CenterInt, 3>& numerator() const noexcept { return numerator_; }
  CenterDen denominator() const noexcept { return denominator_; }
  u8 presentation_arity() const noexcept { return arity_; }
  bool q3_power_i128_certified() const noexcept { return q3_power_i128_; }
  bool orientation_i128_certified() const noexcept { return orientation_i128_; }

 private:
  Point anchor_;
  const std::array<CenterInt, 3>& numerator_;
  CenterDen denominator_;
  u8 arity_;
  bool q3_power_i128_;
  bool orientation_i128_;
};

bool use_native_power(const CenterView& sphere) noexcept {
  return Budget::side <= 127 || sphere.presentation_arity() != 3 || sphere.q3_power_i128_certified();
}

// Precondition interne : use_native_power(sphere). M=2^B, |v_j|<M ; chaque carre et somme de dot<3M^2
// tient en i64. La conversion en i128 est exacte ; |-2*v_j|<2M, avant multiplication par N_j.
// q1 : somme absolue <3M^2. q2 : premier terme <6M^2, chacun des trois suivants <2M^2, total <12M^2.
// q4 : cross(b-a,c-a)_j est le determinant de trois points du MEME carre [0,M-1]^2. Multiaffine,
// son maximum absolu est aux coins, ou il vaut 0 ou (M-1)^2 : donc <M^2, pas pour deux Vec arbitraires.
// Cramer donne D<6M^3 et |N_j|<9M^4 : chacun des quatre termes <18M^5, somme des magnitudes <72M^5.
// q3 sans certificat : uniquement si Budget::side<=127 ; D<24M^4, |N_j|<24M^5, total <216M^6.
// q3 certifie : D<2^(123-2B), |N_j|<2^(124-B). Terme quadratique <3*2^123, chaque lineaire <2^125.
// La somme des magnitudes <15*2^123<2^127 borne les produits ET toutes les sommes partielles, pour tout Point.
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

// Precondition : use_native_power(sphere). Memes majorants que native_power pour chaque borne (voir
// center_power_bounds) : produits et sommes partielles exacts en i128, valeurs dans Budget::side.
std::array<i128, 2> native_bounds(const CenterView& sphere, const BoundTerms& terms) noexcept {
  i128 lower = sphere.denominator() * i128{terms.norm_lower};
  i128 upper = sphere.denominator() * i128{terms.norm_upper};
  for (int j = 0; j < 3; ++j) {
    lower += sphere.numerator()[j] * terms.linear_lower[j];
    upper += sphere.numerator()[j] * terms.linear_upper[j];
  }
  return {lower, upper};
}

Result<PowerBounds> center_power_bounds(const CenterView& sphere, const Box& box) noexcept {
  const auto terms = bound_terms(sphere, box);
  // D>0. Separer les extrema peut elargir l'intervalle, jamais l'inverser ou supprimer un contact.
  // Les quatre termes de CHAQUE borne ont les memes majorants absolus que native_power, y compris le
  // certificat global q3 : norme<3M^2 et facteurs<2M pour chaque extremite Point, meme centre exterieur.
  // Sinon <12M^2 pour q2,
  // <72M^5 pour q4 grace aux cross a ancrage commun, <216M^6 pour q3. Toute somme partielle est bornee ainsi.
  static_assert(2 * kCoordBits + 4 <= 127 && 5 * kCoordBits + 7 <= 127);
  static_assert(Budget::side == 6 * kCoordBits + 8 && 5 * kCoordBits + 7 <= Budget::side);
  if (use_native_power(sphere)) {
    const auto [lower, upper] = native_bounds(sphere, terms);
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
  if (center.orientation_i128_certified()) {
    static_assert(Budget::center_orientation >= 127);  // La borne native implique aussi le budget public.
    // D<2^(124-3B), |N_j|<2^(124-2B), |offset_j|<2^B : chaque D*offset et N tient,
    // |coordinate|<2^(125-2B). Le cross de TROIS Point a meme ancrage a |normal_j|<2^(2B) :
    // determinant multiaffine dans un carre de cote M-1, maxima aux coins, valeurs 0 ou +/-(M-1)^2.
    // Chaque produit <2^125 ; somme des magnitudes <3*2^125<2^127, donc chaque somme partielle tient.
    // Cette preuve ne s'applique PAS a deux Vec arbitraires ni aux autres predicats du centre.
    i128 native_total = 0;
    for (int j = 0; j < 3; ++j) {
      const i128 coordinate = center.numerator()[j] + center.denominator() * offset[j];
      native_total += coordinate * normal[j];
    }
    return detail::sign(native_total);
  }
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
Result<SideInt> power(const Q3Candidate& sphere, Point point) noexcept {
  return center_power(CenterView(sphere), point);
}
Result<SideInt> power(const Q4Candidate& sphere, Point point) noexcept {
  return center_power(CenterView(sphere), point);
}
Result<int> side(const Sphere& sphere, Point point) noexcept {
  return center_side(CenterView(sphere), point);
}
Result<int> side(const Q3Candidate& sphere, Point point) noexcept {
  return center_side(CenterView(sphere), point);
}
Result<int> side(const Q4Candidate& sphere, Point point) noexcept {
  return center_side(CenterView(sphere), point);
}
Result<PowerBounds> power_bounds(const Sphere& sphere, const Box& box) noexcept {
  return center_power_bounds(CenterView(sphere), box);
}
Result<PowerBoundSigns> power_bound_signs(const Sphere& sphere, const Box& box) noexcept {
  const CenterView view(sphere);
  if (use_native_power(view)) {
    // Comme center_side : la voie native certifiee tient dans Budget::side, require_fit ne refuse jamais ;
    // seul l'ordre des bornes reste controle, comme checked_bounds.
    const auto [lower, upper] = native_bounds(view, bound_terms(view, box));
    if (lower > upper) return fail(Reason::arithmetic_invariant);
    return PowerBoundSigns{detail::sign(lower), detail::sign(upper)};
  }
  auto bounds = center_power_bounds(view, box);
  if (!bounds.ok()) return bounds.outcome();
  return PowerBoundSigns{to_wide(bounds.value().lower).sign(), to_wide(bounds.value().upper).sign()};
}

namespace {
// Plancher exact de C/D pour D>0 : C++ tronque vers zero, le reste negatif est ramene dans [0,D).
struct FloorDivision { i128 quotient, remainder; };
FloorDivision floor_division(i128 numerator, i128 denominator) noexcept {
  i128 quotient = numerator / denominator;  // une seule division large ; |quotient*D| <= |numerator|, aucun depassement
  i128 remainder = numerator - quotient * denominator;
  if (remainder < 0) { --quotient; remainder += denominator; }
  return {quotient, remainder};
}
}  // namespace

LatticeSphere::LatticeSphere(const Sphere& sphere) noexcept
    : sphere_(sphere), lattice_(use_native_power(CenterView(sphere))) {
  if (!lattice_) return;
  static_assert(Budget::global_center_numerator <= 126 && Budget::center_denominator + 1 <= 126);
  constexpr i64 m = i64{1} << kCoordBits;
  const i128 d = sphere.denominator();  // D>0 pour toute Sphere fabriquee
  denominator_ = d;
  for (int j = 0; j < 3; ++j) {
    anchor_[j] = sphere.anchor().coordinates()[j];
    numerator_[j] = sphere.numerator()[j];
  }
  for (int j = 0; j < 3; ++j) {
    // Comme compare_centers : |a_j D|<2^(5B+5) et |N_j|<2^(5B+5), donc C_j=a_j D+N_j<2^(5B+6)<=2^126.
    const i128 c = i128{sphere.anchor().coordinates()[j]} * d + sphere.numerator()[j];
    const auto [whole, remainder] = floor_division(c, d);  // c=whole*D+remainder, 0<=remainder<D
    // Saturer le plancher avant tout produit : au-dela de [-2,M+1], ni le point proche ramene dans une boite de
    // [0,M-1] ni le coin eloigne ne changent plus. 2*remainder<2D<2^(4B+6) tient dans i128.
    const i64 q = whole < -2 ? -2 : whole > m + 1 ? m + 1 : static_cast<i64>(whole);
    const i64 nearest = q + (2 * remainder > d ? 1 : 0);  // ex aequo 2r=D : le plus petit, meme distance
    const i64 twice = 2 * q + (remainder == 0 ? 0 : 2 * remainder <= d ? 1 : 2);  // ceil(2C/D)
    nearest_[j] = std::clamp<i64>(nearest, 0, m - 1);
    far_threshold_[j] = std::clamp<i64>(twice, 0, 2 * m - 1);
  }
}

// Precondition : lattice_. Meme expression et meme ordre que native_power ; les points evalues sont des points du
// domaine (sommets ou points entiers d'une Box, sites d'un Point), donc ses majorants valent ici a l'identique.
i128 LatticeSphere::power_at(const std::array<i64, 3>& point) const noexcept {
  const std::array<i64, 3> v{point[0] - anchor_[0], point[1] - anchor_[1], point[2] - anchor_[2]};
  i128 power = denominator_ * i128{detail::dot(v, v)};
  for (int j = 0; j < 3; ++j) power += numerator_[j] * (-2 * i128{v[j]});
  return power;
}

Result<PowerBoundSigns> LatticeSphere::bound_signs(const Box& box) const noexcept {
  if (!lattice_) return power_bound_signs(sphere_, box);
  const auto lo = box.lo().coordinates(), hi = box.hi().coordinates();  // copies : lo()/hi() rendent des valeurs
  std::array<i64, 3> near{}, far{};
  for (int j = 0; j < 3; ++j) {
    near[j] = std::clamp<i64>(nearest_[j], lo[j], hi[j]);
    far[j] = i64{lo[j]} + hi[j] >= far_threshold_[j] ? hi[j] : lo[j];
  }
  // Deux points de la boite fermee, donc du domaine : la puissance native garde ses budgets, pour tout Point.
  const i128 lower = power_at(near);
  if (lower > 0) return PowerBoundSigns{.lower = 1, .upper = 1};
  const i128 upper = power_at(far);
  if (lower > upper) return fail(Reason::arithmetic_invariant);
  return PowerBoundSigns{.lower = detail::sign(lower), .upper = detail::sign(upper)};
}

Result<int> LatticeSphere::side(Point point) const noexcept {
  if (!lattice_) return center_side(CenterView(sphere_), point);
  const auto c = point.coordinates();
  return detail::sign(power_at({i64{c[0]}, i64{c[1]}, i64{c[2]}}));
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
Result<int> orientation(Point a, Point b, Point c, const Q3Candidate& center) noexcept {
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

TriangleKind classify_triangle(Point a, Point b, Point c) noexcept {
  // Les trois angles stricts excluent toute dependance affine, y compris les points confondus.
  // Sinon cross==0 distingue l'alignement du triangle droit/obtus. M=2^B : chaque dot et ses
  // sommes partielles sont <3M^2 en valeur absolue, chaque cross <2M^2 ; aucun carre de cross.
  static_assert(Budget::dot <= 63 && Budget::cross <= 63);
  if (strictly_acute(a, b, c)) return TriangleKind::strict;
  const auto normal = detail::cross(detail::difference(b, a), detail::difference(c, a));
  if (normal[0] == 0 && normal[1] == 0 && normal[2] == 0) return TriangleKind::degenerate;
  return TriangleKind::non_strict;
}

Result<bool> strictly_inside(const Sphere& center, Point a, Point b, Point c, Point d) noexcept {
  return center_inside(CenterView(center), a, b, c, d);
}
Result<bool> strictly_inside(const Q3Candidate& center, Point a, Point b, Point c, Point d) noexcept {
  return center_inside(CenterView(center), a, b, c, d);
}
Result<bool> strictly_inside(const Q4Candidate& center, Point a, Point b, Point c, Point d) noexcept {
  return center_inside(CenterView(center), a, b, c, d);
}
bool is_midpoint(const Sphere& center, Point a, Point b) noexcept {
  return center_midpoint(CenterView(center), a, b);
}
bool is_midpoint(const Q3Candidate& center, Point a, Point b) noexcept {
  return center_midpoint(CenterView(center), a, b);
}
bool is_midpoint(const Q4Candidate& center, Point a, Point b) noexcept {
  return center_midpoint(CenterView(center), a, b);
}

}  // namespace mhgp11::num
