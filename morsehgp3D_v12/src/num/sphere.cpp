// Centres R2 865f5e6 : q2 milieu, q3 double produit vectoriel, q4 Cramer ; D normalise positif.
// Level q3 emploie le produit des trois carres de longueurs / (4|u x v|^2), pour eviter le degre 10 de |N3|^2.
// Repere local (v12, docs/CONTRAT_NUMERIQUE.md, paragraphe 3) : la fabrique lit l'etendue s de sa presentation.
// Jusqu'au palier moyen (s <= 24) les formules de la v11 en i64/i128, dont chaque budget est celui du palier ; au
// palier large (25 <= s <= 32, profil 32 seulement) les memes formules en entiers exacts de 320 bits (detail::Exact).
// Les deux certificats de domaine (CST-0201) sont calcules ici, une fois, sur les coefficients exacts.
#include "num/geometry_internal.hpp"
#include "num/power_certificate.hpp"
#include "num/orientation_certificate.hpp"
#include "num/q4_weights.hpp"

namespace mhgp12::num {
namespace {

// Certificats de domaine de coefficients deja exacts ; hors i128, aucun certificat (ils exigent D < 2^123).
CenterDomains domains_of(const CenterDen& denominator, const std::array<CenterInt, 3>& numerator) noexcept {
  const auto d = detail::as_i128(denominator);
  std::array<i128, 3> n{};
  for (int j = 0; j < 3; ++j) {
    const auto value = detail::as_i128(numerator[j]);
    if (!d || !value) return {};
    n[j] = *value;
  }
  return {static_cast<i8>(detail::power_domain_i128(*d, n)), static_cast<i8>(detail::orientation_domain_i128(*d, n))};
}

struct Stored {
  std::array<CenterInt, 3> numerator{};
  CenterDen denominator{};
};
// Coefficients d'une voie de calcul ramenes au stockage du profil ; un depassement du budget est un invariant.
template <class N, class D>
Result<Stored> store(const std::array<N, 3>& numerator, const D& denominator) noexcept {
  Stored out;
  for (int j = 0; j < 3; ++j) {
    auto value = detail::store_numerator(numerator[j]);
    if (!value.ok()) return value.outcome();
    out.numerator[j] = value.value();
  }
  auto value = detail::store_denominator(denominator);
  if (!value.ok()) return value.outcome();
  out.denominator = value.value();
  return out;
}

// 2D, denominateur du niveau q3 : un bit de plus que le budget de stockage de D.
using DoubledDen = Wide<words_for(DomainBudget::center_denominator + 1)>;
Result<DoubledDen> doubled(const CenterDen& denominator) noexcept {
  DoubledDen value, out;
  if (!resize(to_wide(denominator), value) || !add(value, value, out)) return fail(Reason::arithmetic_invariant);
  return out;
}

// Niveau q4 |N|^2 / D^2 a la largeur du palier du support : chaque carre < 2^(2*numerator4), somme < 3 fois cette
// borne, D^2 < 2^(2*denominator4) ; Words couvre level_numerator du palier.
template <int Words>
Result<Level> q4_level(const std::array<CenterInt, 3>& numerator, const CenterDen& denominator) noexcept {
  Wide<Words> sum;
  for (const auto& coordinate : numerator) {
    auto square = detail::product<Words>(coordinate, coordinate);
    if (!square.ok()) return square.outcome();
    auto next = detail::require_add(sum, square.value());
    if (!next.ok()) return next.outcome();
    sum = next.value();
  }
  auto square = detail::product<Words>(denominator, denominator);
  if (!square.ok()) return square.outcome();
  return detail::checked_level(sum, square.value());
}

}  // namespace

Result<Point> Point::make(i64 x, i64 y, i64 z) noexcept {
  if (x < 0 || y < 0 || z < 0 || x > kCoordMax || y > kCoordMax || z > kCoordMax)
    return fail(Reason::coordinate_out_of_domain);
  return Point({static_cast<u32>(x), static_cast<u32>(y), static_cast<u32>(z)});
}

Result<Box> Box::make(Point lo, Point hi) noexcept {
  for (int j = 0; j < 3; ++j)
    if (lo.coordinates()[j] > hi.coordinates()[j]) return fail(Reason::parameter_out_of_range);
  return Box(lo, hi);
}

Sphere Sphere::point(Point a) noexcept {
  const CenterDen one = integer_one<DomainBudget::center_denominator>();
  return Sphere(a, {}, one, Level{}, 1, 0, domains_of(one, {}));
}

Result<std::optional<Sphere>> Sphere::through(Point a, Point b) noexcept {
  if (a == b) return std::optional<Sphere>{};
  const auto u = detail::difference(b, a);
  const u8 span = detail::presentation_span(a, b);
  // |u|^2 < 3 M^2 : i64 jusqu'a s = 30, i128 au-dela (profil 32).
  const i128 norm = span <= 30 ? i128{detail::dot(u, u)} : detail::dot128(u, u);
  auto level = detail::checked_level(to_wide(norm), to_wide(i64{4}));
  if (!level.ok()) return level.outcome();
  auto coefficients = store(u, i64{2});
  if (!coefficients.ok()) return coefficients.outcome();
  const auto& [n, d] = coefficients.value();
  return std::optional<Sphere>{Sphere(a, n, d, level.value(), 2, span, domains_of(d, n))};
}

Result<std::optional<Sphere>> Sphere::through(Point a, Point b, Point c) noexcept {
  auto candidate = Q3Candidate::through(a, b, c);
  if (!candidate.ok()) return candidate.outcome();
  if (!candidate.value()) return std::optional<Sphere>{};
  auto sphere = candidate.value()->materialize();
  if (!sphere.ok()) return sphere.outcome();
  return std::optional<Sphere>{sphere.value()};
}

Result<std::optional<Q3Candidate>> Q3Candidate::through(Point a, Point b, Point c, LaneCount* lanes) noexcept {
  const u8 span = detail::presentation_span(a, b, c);
  count_lane(lanes, span > kMediumSpan ? Lane::wide : Lane::native);
  if (span > kMediumSpan) return through_exact(a, b, c, span);
  // Paliers etroit et moyen : cross < 2 M^2 et dot < 3 M^2 en i64 ; g < 12 M^4, t_j < 6 M^3, N_j < 24 M^5 en i128.
  const auto u = detail::difference(b, a), v = detail::difference(c, a);
  const auto w = detail::cross(u, v);
  const i128 g = i128{w[0]} * w[0] + i128{w[1]} * w[1] + i128{w[2]} * w[2];
  if (g == 0) return std::optional<Q3Candidate>{};
  const auto uu = detail::dot(u, u), vv = detail::dot(v, v);
  std::array<i128, 3> t{};
  for (int j = 0; j < 3; ++j) t[j] = i128{uu} * v[j] - i128{vv} * u[j];
  static_assert(TierBudgets<Tier::medium>::numerator3 <= 127 && TierBudgets<Tier::medium>::denominator3 <= 127);
  const std::array<i128, 3> n = {t[1] * w[2] - t[2] * w[1], t[2] * w[0] - t[0] * w[2], t[0] * w[1] - t[1] * w[0]};
  auto coefficients = store(n, 2 * g);
  if (!coefficients.ok()) return coefficients.outcome();
  const CenterDomains domains{static_cast<i8>(detail::power_domain_i128(2 * g, n)),
                              static_cast<i8>(detail::orientation_domain_i128(2 * g, n))};
  return std::optional<Q3Candidate>{
      Q3Candidate(a, b, c, coefficients.value().numerator, coefficients.value().denominator, span, domains)};
}

// Palier large : normale < 2 M^2 en i128, g = |w|^2 < 12 M^4, t_j < 6 M^3, N_j < 24 M^5, D = 2g : 165 bits a s = 32.
Result<std::optional<Q3Candidate>> Q3Candidate::through_exact(Point a, Point b, Point c, u8 span) noexcept {
  using detail::Exact;
  const auto u = detail::difference(b, a), v = detail::difference(c, a);
  const auto w = detail::cross128(u, v);
  const Exact g = Exact(w[0]) * Exact(w[0]) + Exact(w[1]) * Exact(w[1]) + Exact(w[2]) * Exact(w[2]);
  if (g.overflow) return fail(Reason::arithmetic_invariant);
  if (g.sign() == 0) return std::optional<Q3Candidate>{};
  const Exact uu(detail::dot128(u, u)), vv(detail::dot128(v, v));
  std::array<Exact, 3> t{};
  for (int j = 0; j < 3; ++j) t[j] = uu * Exact(i128{v[j]}) - vv * Exact(i128{u[j]});
  const std::array<Exact, 3> n{t[1] * Exact(w[2]) - t[2] * Exact(w[1]), t[2] * Exact(w[0]) - t[0] * Exact(w[2]),
                               t[0] * Exact(w[1]) - t[1] * Exact(w[0])};
  const Exact d = g + g;
  if (d.overflow || n[0].overflow || n[1].overflow || n[2].overflow) return fail(Reason::arithmetic_invariant);
  auto coefficients = store(std::array<Wide<5>, 3>{n[0].value, n[1].value, n[2].value}, d.value);
  if (!coefficients.ok()) return coefficients.outcome();
  const auto& [numerator, denominator] = coefficients.value();
  return std::optional<Q3Candidate>{
      Q3Candidate(a, b, c, numerator, denominator, span, domains_of(denominator, numerator))};
}

Result<Sphere> Q3Candidate::materialize() const noexcept {
  // Memes produits qu'avant le report : g>0 deja certifie par la fabrique (D=2g), aucune reduction ni reancrage.
  // |u|^2|v|^2|c-b|^2 < 27 M^6 : deux carres en i128 jusqu'a s = 30, entiers exacts au-dela.
  const auto u = detail::difference(second_, anchor_), v = detail::difference(third_, anchor_);
  const auto bc = detail::difference(third_, second_);
  Wide<5> numerator;
  if (support_span_ <= 30) {
    if (!resize(multiply(to_wide(i128{detail::dot(u, u)} * detail::dot(v, v)), to_wide(detail::dot(bc, bc))),
                numerator)) return fail(Reason::arithmetic_invariant);
  } else {
    using detail::Exact;
    const Exact value = Exact(detail::dot128(u, u)) * Exact(detail::dot128(v, v)) * Exact(detail::dot128(bc, bc));
    if (value.overflow) return fail(Reason::arithmetic_invariant);
    numerator = value.value;
  }
  auto denominator = doubled(denominator_);
  if (!denominator.ok()) return denominator.outcome();
  auto level = detail::checked_level(numerator, denominator.value());
  if (!level.ok()) return level.outcome();
  return Sphere(anchor_, numerator_, denominator_, level.value(), 3, support_span_, domains_);
}

Result<std::optional<Sphere>> Sphere::through(Point a, Point b, Point c, Point d) noexcept {
  auto candidate = Q4Candidate::through(a, b, c, d);
  if (!candidate.ok()) return candidate.outcome();
  if (!candidate.value()) return std::optional<Sphere>{};
  auto sphere = candidate.value()->materialize();
  if (!sphere.ok()) return sphere.outcome();
  return std::optional<Sphere>{sphere.value()};
}

Result<std::optional<Q4Candidate>> Q4Candidate::through(Point a, Point b, Point c, Point d,
                                                       LaneCount* lanes) noexcept {
  const u8 span = detail::presentation_span(a, b, c, d);
  count_lane(lanes, span > kMediumSpan ? Lane::wide : Lane::native);
  if (span > kMediumSpan) return through_exact(a, b, c, d, span);
  // M=2^s, s <= 24 : chaque cross <2M^2 et norm2 <3M^2 ; chaque produit de N <6M^4 et chaque
  // somme partielle <18M^4. |D|<12M^3 ; ses produits, ses sommes et les negations tiennent en i128.
  const auto u = detail::difference(b, a), v = detail::difference(c, a), s = detail::difference(d, a);
  const auto vs = detail::cross(v, s), su = detail::cross(s, u), uv = detail::cross(u, v);
  const i128 det = i128{u[0]} * vs[0] + i128{u[1]} * vs[1] + i128{u[2]} * vs[2];
  if (det == 0) return std::optional<Q4Candidate>{};
  const auto uu = detail::dot(u, u), vv = detail::dot(v, v), ss = detail::dot(s, s);
  static_assert(TierBudgets<Tier::medium>::numerator4 <= 127 && TierBudgets<Tier::medium>::denominator4 <= 127);
  std::array<i128, 3> n{};
  for (int j = 0; j < 3; ++j) n[j] = i128{uu} * vs[j] + i128{vv} * su[j] + i128{ss} * uv[j];
  // Le signe de det s'annule dans les poids seulement avec le numerateur BRUT.
  auto strict = detail::q4_presentation_inside({a,b,c,d}, n, det, vs, su, uv);
  if (!strict.ok()) return strict.outcome();
  i128 denominator = 2 * det;
  if (denominator < 0) {
    denominator = -denominator;
    for (auto& coordinate : n) coordinate = -coordinate;
  }
  auto coefficients = store(n, denominator);
  if (!coefficients.ok()) return coefficients.outcome();
  const CenterDomains domains{static_cast<i8>(detail::power_domain_i128(denominator, n)),
                              static_cast<i8>(detail::orientation_domain_i128(denominator, n))};
  return std::optional<Q4Candidate>{Q4Candidate(a, coefficients.value().numerator, coefficients.value().denominator,
                                                span, domains, strict.value())};
}

// Palier large : normales < 2 M^2 en i128, |det| < 6 M^3 en i128, N_j < 18 M^4 en entiers exacts.
Result<std::optional<Q4Candidate>> Q4Candidate::through_exact(Point a, Point b, Point c, Point d, u8 span) noexcept {
  using detail::Exact;
  const auto u = detail::difference(b, a), v = detail::difference(c, a), s = detail::difference(d, a);
  const auto vs = detail::cross128(v, s), su = detail::cross128(s, u), uv = detail::cross128(u, v);
  const i128 det = u[0] * vs[0] + u[1] * vs[1] + u[2] * vs[2];
  if (det == 0) return std::optional<Q4Candidate>{};
  const Exact uu(detail::dot128(u, u)), vv(detail::dot128(v, v)), ss(detail::dot128(s, s));
  std::array<Exact, 3> n{};
  for (int j = 0; j < 3; ++j) n[j] = uu * Exact(vs[j]) + vv * Exact(su[j]) + ss * Exact(uv[j]);
  auto strict = detail::q4_presentation_inside_exact(n, det, vs, su, uv);
  if (!strict.ok()) return strict.outcome();
  Exact denominator(2 * det);
  if (det < 0) {
    denominator = Exact{} - denominator;
    for (auto& coordinate : n) coordinate = Exact{} - coordinate;
  }
  if (denominator.overflow || n[0].overflow || n[1].overflow || n[2].overflow)
    return fail(Reason::arithmetic_invariant);
  auto coefficients = store(std::array<Wide<5>, 3>{n[0].value, n[1].value, n[2].value}, denominator.value);
  if (!coefficients.ok()) return coefficients.outcome();
  const auto& [numerator, stored] = coefficients.value();
  return std::optional<Q4Candidate>{Q4Candidate(a, numerator, stored, span, domains_of(stored, numerator),
                                                strict.value())};
}

Result<Sphere> Q4Candidate::materialize() const noexcept {
  // Meme somme de carres et meme D^2 qu'avant le decoupage : aucune reduction PGCD ni reancrage.
  static_assert(2 * TierBudgets<Tier::narrow>::numerator4 + 2 <= 64 * 3);
  static_assert(2 * TierBudgets<Tier::medium>::numerator4 + 2 <= 64 * 4);
  static_assert(2 * TierBudgets<Tier::wide>::numerator4 + 2 <= 64 * 5);
  const Tier tier = tier_of(support_span_);
  auto level = tier == Tier::narrow ? q4_level<3>(numerator_, denominator_)
             : tier == Tier::medium ? q4_level<4>(numerator_, denominator_) : q4_level<5>(numerator_, denominator_);
  if (!level.ok()) return level.outcome();
  return Sphere(anchor_, numerator_, denominator_, level.value(), 4, support_span_, domains_, q4_presentation_inside_);
}

}  // namespace mhgp12::num
