// Predicats R2 portes avec budgets par expression. Pas de conversion flottante, pas de filtre heuristique.
// Repere local (v12, docs/CONTRAT_NUMERIQUE.md, paragraphe 3) : chaque predicat lit l'etendue u de son repere
// (support de la sphere et arguments, autour de l'ancre) et prend la voie de son palier : native garantie par le
// palier, native certifiee (certificat de domaine), essai controle, ou large a la largeur du palier. Les voies donnent
// toutes la valeur exacte ; seule la largeur des intermediaires change, et chacune peut etre comptee (LaneCount).
#include "num/center_view.hpp"
#include "num/power_checked.hpp"

#include <algorithm>

namespace mhgp12::num {
namespace {
using detail::CenterView;

// Requete autour de l'ancre : ecart v et son etendue t (|v_j| < 2^t <= 2^32).
struct Query {
  detail::Vec v;
  int t;
};
Query query_of(const CenterView& sphere, Point point) noexcept {
  const auto v = detail::difference(point, sphere.anchor());
  return {v, detail::span_of(v)};
}

// Precondition : power_lane native ou certifiee. Natif par palier : q1 < 3M^2, q2 < 12M^2, q4 < 72M^5 (normales a
// ancrage commun dans le cube du repere, D < 6M^3, |N_j| < 9M^4), q3 etroit < 216M^6, avec M = 2^u. Certifie (domaine
// t) : terme quadratique < 3*2^123, chaque lineaire < 2^125, somme des magnitudes < 15*2^123 < 2^127. Ces sommes
// majorent CHAQUE produit et somme partielle, sans annulation ni convexite. Norme en i64 jusqu'a t = 30.
i128 native_power(const CenterView& sphere, const Query& q) noexcept {
  const i128 norm = q.t <= 30 ? i128{detail::dot(q.v, q.v)} : detail::dot128(q.v, q.v);
  i128 total = sphere.d128() * norm;
  for (int j = 0; j < 3; ++j) total += sphere.n128()[j] * (-2 * i128{q.v[j]});
  return total;
}

template <int Words>
Result<SideInt> wide_power(const CenterView& sphere, const Query& q) noexcept {
  auto first = detail::product<Words>(sphere.denominator(), detail::dot128(q.v, q.v));
  if (!first.ok()) return first.outcome();
  auto total = first.value();
  for (int j = 0; j < 3; ++j) {
    auto term = detail::product<Words>(sphere.numerator()[j], -2 * i128{q.v[j]});
    if (!term.ok()) return term.outcome();
    auto sum = detail::require_add(total, term.value());
    if (!sum.ok()) return sum.outcome();
    total = sum.value();
  }
  return detail::require_fit<DomainBudget::side>(total);
}
// Voie large a la largeur du palier de u (6u+8 au plafond du palier).
Result<SideInt> wide_power(const CenterView& sphere, const Query& q) noexcept {
  const int words = detail::power_words(sphere, q.t);
  return words <= 2 ? wide_power<2>(sphere, q) : words == 3 ? wide_power<3>(sphere, q) : wide_power<4>(sphere, q);
}

std::optional<i128> checked_power(const CenterView& sphere, const Query& q) noexcept {
  // Operandes deja en i128 : norme < 3*2^66, facteurs -2v_j < 2^34 ; aucune conversion retrecissante avant les builtins.
  const std::array<i128, 3> factors{-2 * i128{q.v[0]}, -2 * i128{q.v[1]}, -2 * i128{q.v[2]}};
  return detail::checked_power_sum(sphere.d128(), sphere.n128(), detail::dot128(q.v, q.v), factors);
}

Result<SideInt> center_power(const CenterView& sphere, Point point, LaneCount* lanes) noexcept {
  const Query q = query_of(sphere, point);
  const Lane lane = detail::power_lane(sphere, q.t);
  if (lane == Lane::native || lane == Lane::certified) {
    count_lane(lanes, lane);
    return detail::require_fit<DomainBudget::side>(to_wide(native_power(sphere, q)));
  }
  if (lane == Lane::checked) {
    if (const auto value = checked_power(sphere, q)) {
      count_lane(lanes, Lane::checked);
      return detail::require_fit<DomainBudget::side>(to_wide(*value));
    }
  }
  count_lane(lanes, Lane::wide);
  return wide_power(sphere, q);
}

Result<int> center_side(const CenterView& sphere, Point point, LaneCount* lanes) noexcept {
  const Query q = query_of(sphere, point);
  const Lane lane = detail::power_lane(sphere, q.t);
  if (lane == Lane::native || lane == Lane::certified) {
    count_lane(lanes, lane);
    return detail::sign(native_power(sphere, q));
  }
  if (lane == Lane::checked) {
    if (const auto value = checked_power(sphere, q)) {
      count_lane(lanes, Lane::checked);
      return detail::sign(*value);
    }
  }
  count_lane(lanes, Lane::wide);
  auto value = wide_power(sphere, q);
  if (!value.ok()) return value.outcome();
  return to_wide(value.value()).sign();
}

struct BoundTerms {
  i128 norm_lower = 0, norm_upper = 0;
  std::array<i64, 3> linear_lower{}, linear_upper{};
  int t = 0;  // etendue des coins de la boite autour de l'ancre
};

BoundTerms bound_terms(const CenterView& sphere, const Box& box) noexcept {
  const auto lo = detail::difference(box.lo(), sphere.anchor());
  const auto hi = detail::difference(box.hi(), sphere.anchor());
  BoundTerms terms;
  terms.t = std::max(detail::span_of(lo), detail::span_of(hi));
  // |lo_j|,|hi_j| < 2^t <= 2^32 : les carres < 2^64 en i128, leurs trois sommes < 3*2^64, les facteurs doubles < 2^34.
  for (int j = 0; j < 3; ++j) {
    const i128 lo2 = i128{lo[j]} * lo[j], hi2 = i128{hi[j]} * hi[j];
    terms.norm_lower += lo[j] > 0 ? lo2 : hi[j] < 0 ? hi2 : 0;
    terms.norm_upper += lo2 > hi2 ? lo2 : hi2;
    const bool nonnegative = sphere.numerator_sign(j) >= 0;
    terms.linear_lower[j] = -2 * (nonnegative ? hi[j] : lo[j]);
    terms.linear_upper[j] = -2 * (nonnegative ? lo[j] : hi[j]);
  }
  return terms;
}

template <int Words>
Result<PowerBounds> checked_bounds(const Wide<Words>& lower, const Wide<Words>& upper) noexcept {
  if (compare(lower, upper) > 0) return fail(Reason::arithmetic_invariant);
  const auto lo = detail::require_fit<DomainBudget::side>(lower), hi = detail::require_fit<DomainBudget::side>(upper);
  if (!lo.ok()) return lo.outcome();
  if (!hi.ok()) return hi.outcome();
  return PowerBounds{lo.value(), hi.value()};
}

// Precondition : voie native ou certifiee pour l'etendue t des coins. Memes majorants que native_power pour chaque
// borne : chaque extremite est un point d'etendue < 2^t autour de l'ancre, norme < 3*2^(2t), facteurs < 2^(t+1).
std::array<i128, 2> native_bounds(const CenterView& sphere, const BoundTerms& terms) noexcept {
  i128 lower = sphere.d128() * terms.norm_lower;
  i128 upper = sphere.d128() * terms.norm_upper;
  for (int j = 0; j < 3; ++j) {
    lower += sphere.n128()[j] * terms.linear_lower[j];
    upper += sphere.n128()[j] * terms.linear_upper[j];
  }
  return {lower, upper};
}

template <int Words>
Result<PowerBounds> wide_bounds(const CenterView& sphere, const BoundTerms& terms) noexcept {
  auto lower = detail::product<Words>(sphere.denominator(), terms.norm_lower);
  auto upper = detail::product<Words>(sphere.denominator(), terms.norm_upper);
  if (!lower.ok()) return lower.outcome();
  if (!upper.ok()) return upper.outcome();
  for (int j = 0; j < 3; ++j) {
    const auto lo = detail::product<Words>(sphere.numerator()[j], terms.linear_lower[j]);
    const auto hi = detail::product<Words>(sphere.numerator()[j], terms.linear_upper[j]);
    if (!lo.ok()) return lo.outcome();
    if (!hi.ok()) return hi.outcome();
    lower = detail::require_add(lower.value(), lo.value());
    upper = detail::require_add(upper.value(), hi.value());
    if (!lower.ok()) return lower.outcome();
    if (!upper.ok()) return upper.outcome();
  }
  return checked_bounds(lower.value(), upper.value());
}

Result<PowerBounds> center_power_bounds(const CenterView& sphere, const Box& box, LaneCount* lanes) noexcept {
  const auto terms = bound_terms(sphere, box);
  // D>0. Separer les extrema peut elargir l'intervalle, jamais l'inverser ou supprimer un contact.
  const Lane lane = detail::power_lane(sphere, terms.t);
  if (lane == Lane::native || lane == Lane::certified) {
    count_lane(lanes, lane);
    const auto [lower, upper] = native_bounds(sphere, terms);
    return checked_bounds(to_wide(lower), to_wide(upper));
  }
  if (lane == Lane::checked) {
    const std::array<i128, 3> ll{terms.linear_lower[0], terms.linear_lower[1], terms.linear_lower[2]};
    const std::array<i128, 3> lu{terms.linear_upper[0], terms.linear_upper[1], terms.linear_upper[2]};
    const auto native_lower = detail::checked_power_sum(sphere.d128(), sphere.n128(), terms.norm_lower, ll);
    const auto native_upper = detail::checked_power_sum(sphere.d128(), sphere.n128(), terms.norm_upper, lu);
    // Aucun resultat partiel : si l'un des essais refuse, reprendre LES DEUX bornes dans la voie Wide historique.
    if (native_lower && native_upper) {
      count_lane(lanes, Lane::checked);
      return checked_bounds(to_wide(*native_lower), to_wide(*native_upper));
    }
  }
  count_lane(lanes, Lane::wide);
  const int words = detail::power_words(sphere, terms.t);
  return words <= 2 ? wide_bounds<2>(sphere, terms) : words == 3 ? wide_bounds<3>(sphere, terms)
                    : wide_bounds<4>(sphere, terms);
}

template <int Words>
Result<int> wide_orientation(const detail::Vec128& normal, const detail::Vec& offset, const CenterView& center,
                             int bits) noexcept {
  Wide<Words> total;
  for (int j = 0; j < 3; ++j) {
    // Palier moyen : N + D*(anchor-a) < 48 M^5 < 2^(5u+6) <= 2^126, exact en i128.
    const i128 coordinate = center.n128()[j] + center.d128() * offset[j];
    auto term = detail::product<Words>(coordinate, normal[j]);
    if (!term.ok()) return term.outcome();
    auto sum = detail::require_add(total, term.value());
    if (!sum.ok()) return sum.outcome();
    total = sum.value();
  }
  if (total.bit_length() > bits) return fail(Reason::arithmetic_invariant);
  return total.sign();
}

// Palier large : coefficients et coordonnees en entiers exacts (< 288 M^7 < 2^240 a u = 33).
Result<int> exact_orientation(const detail::Vec128& normal, const detail::Vec& offset,
                              const CenterView& center) noexcept {
  using detail::Exact;
  Exact total;
  for (int j = 0; j < 3; ++j)
    total = total + (Exact(to_wide(center.numerator()[j])) +
                     Exact(to_wide(center.denominator())) * Exact(i128{offset[j]})) * Exact(normal[j]);
  if (total.overflow) return fail(Reason::arithmetic_invariant);
  return total.sign();
}

Result<int> center_orientation(Point a, Point b, Point c, const CenterView& center, LaneCount* lanes) noexcept {
  const auto u = detail::difference(b, a), v = detail::difference(c, a);
  const auto offset = detail::difference(center.anchor(), a);
  // Repere : les trois points dans un cube de cote < 2^t, l'ancre a moins de 2^t de a.
  const int t = std::max<int>(detail::presentation_span(a, b, c), detail::span_of(offset));
  const Tier tier = tier_of(std::max(center.span(), t));
  const detail::Vec128 normal = detail::cross128(u, v);  // < 2 M^2 : exact en i128 a toute etendue
  if (tier == Tier::narrow || (center.native_coefficients() && center.orientation_domain() >= t)) {
    count_lane(lanes, tier == Tier::narrow ? Lane::native : Lane::certified);
    // Palier etroit (u <= 16) : |N_j + D offset_j| < 48 M^5, |normal_j| < M^2 (determinant multiaffine dans le cube),
    // somme < 3*48 M^7 < 2^(7u+8) <= 2^120. Certificat t : |N_j + D offset_j| < 2^(125-2t), |normal_j| < 2^(2t),
    // chaque produit < 2^125, somme des magnitudes < 3*2^125 < 2^127. Pas pour deux Vec arbitraires.
    i128 native_total = 0;
    for (int j = 0; j < 3; ++j) {
      const i128 coordinate = center.n128()[j] + center.d128() * offset[j];
      native_total += coordinate * normal[j];
    }
    return detail::sign(native_total);
  }
  count_lane(lanes, Lane::wide);
  if (tier == Tier::medium)
    return wide_orientation<words_for(TierBudgets<Tier::medium>::center_orientation)>(
        normal, offset, center, TierBudgets<Tier::medium>::center_orientation);
  return exact_orientation(normal, offset, center);
}

Result<bool> center_inside(const CenterView& center, Point a, Point b, Point c, Point d, LaneCount* lanes) noexcept {
  const std::array<Point, 4> points{a, b, c, d};
  for (int opposite = 0; opposite < 4; ++opposite) {
    std::array<Point, 3> face{};
    int j = 0;
    for (int i = 0; i < 4; ++i)
      if (i != opposite) face[j++] = points[i];
    const int vertex_sign = detail::sign(orientation(face[0], face[1], face[2], points[opposite]));
    if (vertex_sign == 0) return false;
    auto center_sign = center_orientation(face[0], face[1], face[2], center, lanes);
    if (!center_sign.ok()) return center_sign.outcome();
    if (center_sign.value() != vertex_sign) return false;
  }
  return true;
}

// Test du milieu en forme locale (CST-0114) : 2 N_j = D ((a_j-o_j)+(b_j-o_j)), jamais le centre absolu D a_j + N_j.
bool center_midpoint(const CenterView& center, Point a, Point b, LaneCount* lanes) noexcept {
  const auto da = detail::difference(a, center.anchor()), db = detail::difference(b, center.anchor());
  const Tier tier = tier_of(std::max({center.span(), detail::span_of(da), detail::span_of(db)}));
  if (tier != Tier::wide) {
    // Repere d'etendue u <= 24 : 2|N_j| < 48 M^5 et |D (da_j+db_j)| < 24 M^4 * 2M, donc < 2^(5u+6) <= 2^126.
    static_assert(TierBudgets<Tier::medium>::midpoint_frame <= 127);
    count_lane(lanes, Lane::native);
    for (int j = 0; j < 3; ++j)
      if (2 * center.n128()[j] != center.d128() * (i128{da[j]} + db[j])) return false;
    return true;
  }
  count_lane(lanes, Lane::wide);
  using detail::Exact;
  for (int j = 0; j < 3; ++j) {
    const Exact twice = Exact(to_wide(center.numerator()[j])) + Exact(to_wide(center.numerator()[j]));
    const Exact other = Exact(to_wide(center.denominator())) * Exact(i128{da[j]} + db[j]);
    if (twice.overflow || other.overflow || compare(twice.value, other.value) != 0) return false;
  }
  return true;
}

}  // namespace

DotInt squared_distance(Point a, Point b, LaneCount* lanes) noexcept {
  // Elargir en SIGNE avant chaque soustraction : les u32 de Point ne doivent jamais soustraire en non signe.
  const i64 dx = i64{a.x()} - b.x(), dy = i64{a.y()} - b.y(), dz = i64{a.z()} - b.z();
  // NUM-REQUETE (CST-0110) : |delta| < 2^32, chaque carre < 2^64 tient en u64. Etendue de la paire <= 31 : somme
  // < 3*2^62, native. Sinon somme u64 controlee, recalculee en u128 au debordement (profil 32 seulement).
  const auto magnitude = [](i64 value) noexcept { return static_cast<u64>(value < 0 ? -value : value); };
  const u64 sx = magnitude(dx) * magnitude(dx), sy = magnitude(dy) * magnitude(dy), sz = magnitude(dz) * magnitude(dz);
  if (detail::span_of({dx, dy, dz}) <= 31) {
    count_lane(lanes, Lane::native);
    return static_cast<DotInt>(sx + sy + sz);
  }
  if constexpr (DomainBudget::dot > 63) {
    u64 partial = 0, total = 0;
    if (!__builtin_add_overflow(sx, sy, &partial) && !__builtin_add_overflow(partial, sz, &total)) {
      count_lane(lanes, Lane::checked);
      return static_cast<DotInt>(total);
    }
    count_lane(lanes, Lane::wide);
    return static_cast<DotInt>(u128{sx} + sy + sz);
  } else {
    return static_cast<DotInt>(sx + sy + sz);  // inatteignable : etendue <= B <= 31 sous ce profil
  }
}

Result<SideInt> power(const Sphere& sphere, Point point, LaneCount* lanes) noexcept {
  return center_power(CenterView(sphere), point, lanes);
}
Result<SideInt> power(const Q3Candidate& sphere, Point point, LaneCount* lanes) noexcept {
  return center_power(CenterView(sphere), point, lanes);
}
Result<SideInt> power(const Q4Candidate& sphere, Point point, LaneCount* lanes) noexcept {
  return center_power(CenterView(sphere), point, lanes);
}
Result<int> side(const Sphere& sphere, Point point, LaneCount* lanes) noexcept {
  return center_side(CenterView(sphere), point, lanes);
}
Result<int> side(const Q3Candidate& sphere, Point point, LaneCount* lanes) noexcept {
  return center_side(CenterView(sphere), point, lanes);
}
Result<int> side(const Q4Candidate& sphere, Point point, LaneCount* lanes) noexcept {
  return center_side(CenterView(sphere), point, lanes);
}
Result<PowerBounds> power_bounds(const Sphere& sphere, const Box& box, LaneCount* lanes) noexcept {
  return center_power_bounds(CenterView(sphere), box, lanes);
}
Result<PowerBoundSigns> power_bound_signs(const Sphere& sphere, const Box& box, LaneCount* lanes) noexcept {
  const CenterView view(sphere);
  const auto terms = bound_terms(view, box);
  const Lane lane = detail::power_lane(view, terms.t);
  if (lane == Lane::native || lane == Lane::certified) {
    // Comme center_side : la voie native tient dans le budget, require_fit ne refuse jamais ; seul l'ordre des
    // bornes reste controle, comme checked_bounds.
    count_lane(lanes, lane);
    const auto [lower, upper] = native_bounds(view, terms);
    if (lower > upper) return fail(Reason::arithmetic_invariant);
    return PowerBoundSigns{detail::sign(lower), detail::sign(upper)};
  }
  auto bounds = center_power_bounds(view, box, lanes);
  if (!bounds.ok()) return bounds.outcome();
  return PowerBoundSigns{to_wide(bounds.value().lower).sign(), to_wide(bounds.value().upper).sign()};
}

DeterminantInt orientation(Point a, Point b, Point c, Point d) noexcept {
  const auto u = detail::difference(b, a), v = detail::difference(c, a), w = detail::difference(d, a);
  const auto normal = detail::cross128(u, v);
  // Six monomes < 6 M^3 < 2^(3s+3) <= 2^102 a toute etendue de Point : i128 exact, DeterminantInt le porte.
  static_assert(SpanBudgets<32>::determinant <= 127);
  return static_cast<DeterminantInt>(normal[0] * w[0] + normal[1] * w[1] + normal[2] * w[2]);
}

Result<int> orientation(Point a, Point b, Point c, const Sphere& center, LaneCount* lanes) noexcept {
  return center_orientation(a, b, c, CenterView(center), lanes);
}
Result<int> orientation(Point a, Point b, Point c, const Q3Candidate& center, LaneCount* lanes) noexcept {
  return center_orientation(a, b, c, CenterView(center), lanes);
}
Result<int> orientation(Point a, Point b, Point c, const Q4Candidate& center, LaneCount* lanes) noexcept {
  return center_orientation(a, b, c, CenterView(center), lanes);
}

bool strictly_acute(Point a, Point b, Point c) noexcept {
  // Trois produits scalaires < 3 M^2 : i64 jusqu'a l'etendue 30, i128 au-dela (profil 32).
  if (detail::presentation_span(a, b, c) <= 30)
    return detail::dot(detail::difference(b, a), detail::difference(c, a)) > 0 &&
           detail::dot(detail::difference(a, b), detail::difference(c, b)) > 0 &&
           detail::dot(detail::difference(a, c), detail::difference(b, c)) > 0;
  return detail::dot128(detail::difference(b, a), detail::difference(c, a)) > 0 &&
         detail::dot128(detail::difference(a, b), detail::difference(c, b)) > 0 &&
         detail::dot128(detail::difference(a, c), detail::difference(b, c)) > 0;
}

TriangleKind classify_triangle(Point a, Point b, Point c) noexcept {
  // Les trois angles stricts excluent toute dependance affine, y compris les points confondus.
  // Sinon cross==0 distingue l'alignement du triangle droit/obtus. Chaque cross < 2 M^2, exact en i128.
  if (strictly_acute(a, b, c)) return TriangleKind::strict;
  const auto normal = detail::cross128(detail::difference(b, a), detail::difference(c, a));
  if (normal[0] == 0 && normal[1] == 0 && normal[2] == 0) return TriangleKind::degenerate;
  return TriangleKind::non_strict;
}

Result<bool> strictly_inside(const Sphere& center, Point a, Point b, Point c, Point d) noexcept {
  return center_inside(CenterView(center), a, b, c, d, nullptr);
}
Result<bool> strictly_inside(const Q3Candidate& center, Point a, Point b, Point c, Point d) noexcept {
  return center_inside(CenterView(center), a, b, c, d, nullptr);
}
Result<bool> strictly_inside(const Q4Candidate& center, Point a, Point b, Point c, Point d) noexcept {
  return center_inside(CenterView(center), a, b, c, d, nullptr);
}
bool is_midpoint(const Sphere& center, Point a, Point b, LaneCount* lanes) noexcept {
  return center_midpoint(CenterView(center), a, b, lanes);
}
bool is_midpoint(const Q3Candidate& center, Point a, Point b, LaneCount* lanes) noexcept {
  return center_midpoint(CenterView(center), a, b, lanes);
}
bool is_midpoint(const Q4Candidate& center, Point a, Point b, LaneCount* lanes) noexcept {
  return center_midpoint(CenterView(center), a, b, lanes);
}

}  // namespace mhgp12::num
