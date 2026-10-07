// Ordre lexicographique exact des centres a+N/D, en deux temps (docs/CONTRAT_NUMERIQUE.md, paragraphe 4) : aucune
// positivite ni reduction PGCD supposee, aucun entier de la taille de B. La v11 comparait (a_j D + N_j) D' a
// (a'_j D' + N'_j) D sur 9B+11 bits ; ici, pour chaque axe dans l'ordre x, y, z : la partie entiere
// floor(c_j) = a_j + floor(N_j/D), puis, seulement si elles sont egales, la partie fractionnaire r_j/D (0 <= r_j < D)
// par produits croises r D' contre r' D, compares sans les soustraire. Meme ordre que la v11 : c_j = floor(c_j) + r_j/D.
// Preconditions : D > 0 (signe normalise en q4) ; plancher mathematique (reste ramene dans [0, D)).
#include "num/big.hpp"
#include "num/center_view.hpp"
#include "num/lattice_internal.hpp"
#include "num/power_certificate.hpp"

namespace mhgp12::num {
namespace {

// Partie entiere et reste d'un axe. Coefficients en i128 : floor exact en i128 (|quotient| <= |N_j|), puis + a_j sans
// depassement (|N_j| < 2^127 - 2^32 sous tout certificat ou palier ; sinon voie Big). Coefficients plus larges
// (palier large au profil 32) : division de Big, partie entiere et reste exacts.
struct AxisParts {
  Big whole, remainder;
};
Result<AxisParts> wide_parts(const detail::CenterView& center, int axis) noexcept {
  AxisParts out;
  Big quotient;
  const Big n = Big::from_wide(to_wide(center.numerator()[axis]));
  const Big d = Big::from_wide(to_wide(center.denominator()));
  MHGP12_TRY(divide(n, d, quotient, out.remainder));
  MHGP12_TRY(add(quotient, Big::from_u64(center.anchor().coordinates()[axis]), out.whole));
  return out;
}

// Comparaison des fractions r_a/D_a et r_b/D_b : produits croises au palier du plus large des deux supports (8s+10).
template <int Words, class A, class B>
int fraction_order(const A& ra, const B& db, const A& rb, const B& da) noexcept {
  Wide<Words> left, right;
  const bool fits = multiply_into(to_wide(ra), to_wide(db), left) && multiply_into(to_wide(rb), to_wide(da), right);
  return fits ? compare(left, right) : 0;  // inatteignable : r < D, chaque produit < D_a D_b < 2^(8s+10)
}

int native_axis(const detail::CenterView& a, const detail::CenterView& b, int axis, int span,
                LaneCount* lanes) noexcept {
  const i128 da = a.d128(), db = b.d128();
  const auto [qa, ra] = detail::floor_division(a.n128()[axis], da);
  const auto [qb, rb] = detail::floor_division(b.n128()[axis], db);
  const i128 whole_a = qa + a.anchor().coordinates()[axis], whole_b = qb + b.anchor().coordinates()[axis];
  if (whole_a != whole_b) return whole_a < whole_b ? -1 : 1;
  // r < D < 2^(4s+5) : chaque produit croise < 2^(8s+10), natif si les longueurs le permettent.
  if (detail::magnitude_bits(ra) + detail::magnitude_bits(db) <= 126 &&
      detail::magnitude_bits(rb) + detail::magnitude_bits(da) <= 126) {
    count_lane(lanes, Lane::native);
    const i128 left = ra * db, right = rb * da;
    return (left > right) - (left < right);
  }
  count_lane(lanes, Lane::wide);
  const Tier tier = tier_of(span);
  if (tier == Tier::narrow) return fraction_order<words_for(TierBudgets<Tier::narrow>::center_fraction)>(ra, db, rb, da);
  if (tier == Tier::medium) return fraction_order<words_for(TierBudgets<Tier::medium>::center_fraction)>(ra, db, rb, da);
  return fraction_order<words_for(TierBudgets<Tier::wide>::center_fraction)>(ra, db, rb, da);
}

int wide_axis(const detail::CenterView& a, const detail::CenterView& b, int axis, LaneCount* lanes) noexcept {
  count_lane(lanes, Lane::wide);
  const auto pa = wide_parts(a, axis), pb = wide_parts(b, axis);
  if (!pa.ok() || !pb.ok()) return 0;  // inatteignable : N et D sont dans la capacite de Big
  const int whole = compare(pa.value().whole, pb.value().whole);
  if (whole != 0) return whole;
  Big left, right;
  const Big da = Big::from_wide(to_wide(a.denominator())), db = Big::from_wide(to_wide(b.denominator()));
  if (!multiply(pa.value().remainder, db, left).ok() || !multiply(pb.value().remainder, da, right).ok()) return 0;
  return compare(left, right);
}

// Voie i128 : coefficients en i128 et |N_j| < 2^125, pour que floor(N_j/D) + a_j ne deborde pas (toujours vrai aux
// paliers etroit et moyen, ou |N_j| < 24 M^5 <= 2^125).
bool native_centers(const detail::CenterView& center) noexcept {
  if (!center.native_coefficients()) return false;
  for (const i128 coordinate : center.n128())
    if (detail::magnitude_bits(coordinate) > 125) return false;
  return true;
}

}  // namespace

int compare_centers(const Sphere& a, const Sphere& b, LaneCount* lanes) noexcept {
  const detail::CenterView va(a), vb(b);
  const int span = std::max(va.span(), vb.span());
  const bool native = native_centers(va) && native_centers(vb);
  for (int axis = 0; axis < 3; ++axis) {
    const int order = native ? native_axis(va, vb, axis, span, lanes) : wide_axis(va, vb, axis, lanes);
    if (order != 0) return order;
  }
  return 0;
}

}  // namespace mhgp12::num
