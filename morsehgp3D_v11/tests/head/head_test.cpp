// Portes unitaires du module head (tranche S10, sortie plate) sur des arbres de points abstraits :
//   fixtures   F14a a F14e de bench/points_flat_gate.py (head_fixtures), attendus manuels de clusters retenus :
//              2^-70 (les enfants l'emportent), plateau a gros et petits enfants (feuilles), entree entre deux niveaux
//              qui franchit mcs, n < mcs (tout bruit), foret a racine virtuelle (EOM et feuilles) ;
//   equalities egalites EOM CERTIFIEES (le parent l'emporte) : rationnelle a z = 1 (1/1 + 1/3 = 2 / (3/2)),
//              irrationnelle a z = 1 (1/sqrt 2 + 1/sqrt 8 = 2 / sqrt(32/9), classe de sqrt 2), a z = 2
//              (1 + 1/49 = 2 / (7/5)^2) ; la meme geometrie a l'autre z n'est pas une egalite (une egalite a z = 1 ne
//              prouve rien a z = 2) ; compteurs exact et equalities ;
//   huge       racines hors de 2^100 (source abstraite), dont R = 2^127 - 1 exactement : plateaux sans encadrement,
//              repli exact, egalites certifiees (audits 100fcc12b et 8a89493b2) ;
//   dates      repli exact des dates (port de _inverse_date et _mask_mul, recu eom_exact_audit_20261004) : egalites
//              certifiees phi(date) = phi(niveau) a z = 1, 2, 3 pour (4, 9, 4) contre 9, (8, 2, 8) contre 2
//              (classe de sqrt 2), tous deux a Delta != 0, et (9, 4, 1) contre 16 (Delta = 0) ; date a trois racines sqrt 2 + sqrt 3 - 1
//              encadree strictement par les niveaux 4.6 et 4.61 a z = 1, 2, 3 (identite e^2 (1/e)^2 = 1 vue par
//              les signes) ; date non positive refusee (head_invariant) ;
//   order      etiquettes dans l'ordre d'entree sous PointId arbitraires et permutation ; PointId absent ou repete :
//              head_invariant ;
//   refusals   mcs < 2, z hors de {1, 2, 3} (parameter_out_of_range) ; jonction au niveau nul (head_invariant) ;
//              arbre mal forme (head_invariant) ; budget trop petit (memory_budget), budget rendu.
#include <map>
#include <vector>

#include "head/head.hpp"
#include "head/internal.hpp"
#include "test.hpp"

using namespace mhgp11;

namespace {

using num::Big;
using num::Rational;

Rational frac(i64 a, i64 b = 1) {
  Rational out;
  if (!Rational::make(Big::from_i64(a), Big::from_i64(b), out).ok()) return Rational::from_i64(-1);
  return out;
}

// Niveaux d'une fixture : rang -> rationnel exact.
class Levels final : public head::LevelSource {
 public:
  std::vector<Rational> values;
  u32 add(const Rational& value) {
    values.push_back(value);
    return static_cast<u32>(values.size() - 1);
  }
  Outcome root(u32 rank, u128& out) const noexcept override {
    if (rank >= values.size()) return fail(Reason::head_invariant);
    const Rational& v = values[rank];
    Big scaled, quotient, rest, s;
    MHGP11_TRY(num::shift_left(v.numerator(), 128, scaled));
    MHGP11_TRY(num::divide(scaled, v.denominator(), quotient, rest));
    MHGP11_TRY(num::isqrt(quotient, s));
    const auto value = s.magnitude_u128();
    if (!value) return fail(Reason::arithmetic_invariant);
    out = *value;
    return {};
  }
  Outcome value(u32 rank, Rational& out) const noexcept override {
    if (rank >= values.size()) return fail(Reason::head_invariant);
    out.assign(values[rank]);
    return {};
  }
};

// Arbre abstrait (PointTree de bench/points_flat.py) : plateaux, blocs (plateau, enfants), entrees.
struct Tree {
  Levels levels;
  std::vector<u32> pt, pm, pq, block_plateau, block_parent, site_block, site_plateau;
  std::vector<PointId> ids;
  explicit Tree(u32 n) : site_block(n, kNone), site_plateau(n, kNone) {
    for (u32 i = 0; i < n; ++i) ids.push_back(make_id<PointId>(i));
  }
  // Niveau de rayon x = a / b : l = x^2.
  u32 radius(i64 a, i64 b = 1) { return square(frac(a * a, b * b)); }
  u32 square(const Rational& l) {
    const u32 r = levels.add(l);
    pt.push_back(r);
    pm.push_back(r);
    pq.push_back(r);
    return static_cast<u32>(pt.size() - 1);
  }
  u32 date(const Rational& t, const Rational& m, const Rational& q) {
    pt.push_back(levels.add(t));
    pm.push_back(levels.add(m));
    pq.push_back(levels.add(q));
    return static_cast<u32>(pt.size() - 1);
  }
  u32 block(u32 plateau, std::vector<u32> children = {}) {
    const u32 b = static_cast<u32>(block_plateau.size());
    block_plateau.push_back(plateau);
    block_parent.push_back(kNone);
    for (const u32 c : children) block_parent[c] = b;
    return b;
  }
  void enter(u32 site, u32 block, u32 plateau) {
    site_block[site] = block;
    site_plateau[site] = plateau;
  }
  head::TreeView view() const {
    return {pt, pm, pq, block_plateau, block_parent, site_block, site_plateau, ids};
  }
};

// Groupes d'etiquettes, dans l'ordre du premier site (groups de points_flat_gate.py).
std::vector<std::vector<u32>> groups(std::span<const i64> labels) {
  std::vector<std::vector<u32>> out;
  std::map<i64, u64> where;
  for (u32 s = 0; s < labels.size(); ++s) {
    if (labels[s] < 0) continue;
    auto [it, fresh] = where.emplace(labels[s], out.size());
    if (fresh) out.emplace_back();
    out[it->second].push_back(s);
  }
  return out;
}

using Groups = std::vector<std::vector<u32>>;

Result<head::SiteLabels> run(const Tree& t, u32 mcs, u32 z, head::Selection selection) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  return head::flat_sites(t.view(), t.levels, {mcs, z, selection}, budget);
}

bool same_groups(const Result<head::SiteLabels>& got, const Groups& expected) {
  return got.ok() && groups(got.value().labels.span()) == expected;
}

// F14a : enfants D1 = {0, 1}, D2 = {2, 3} rejoints au rayon a, fusion au rayon 2, haut au rayon 4 sous une racine ;
// S(D1) + S(D2) - S(P) = 4 (1/a - 2/2 + 1/4) = 2^-70 > 0 (1/a = 3/4 + 2^-72).
Tree f14a() {
  Tree t(6);
  Rational inv_a;
  const Big two72 = [] {
    Big out;
    static_cast<void>(num::shift_left(Big::from_u64(1), 72, out));
    return out;
  }();
  Big num_a, den_a;  // 1/a = (3 2^70 + 1) / 2^72 ; a^2 = 2^144 / (3 2^70 + 1)^2
  Big three70;
  static_cast<void>(num::shift_left(Big::from_u64(3), 70, three70));
  static_cast<void>(num::add(three70, Big::from_u64(1), den_a));
  static_cast<void>(num::multiply(den_a, den_a, den_a));
  static_cast<void>(num::multiply(two72, two72, num_a));
  Rational a2;
  static_cast<void>(Rational::make(num_a, den_a, a2));
  const u32 pa = t.square(a2), p2 = t.radius(2), p3 = t.radius(3), p4 = t.radius(4);
  const u32 b0 = t.block(pa), b1 = t.block(pa), b2 = t.block(p2, {b0, b1}), b3 = t.block(p3), b4 = t.block(p4, {b2, b3});
  static_cast<void>(b4);
  t.enter(0, b0, pa);
  t.enter(1, b0, pa);
  t.enter(2, b1, pa);
  t.enter(3, b1, pa);
  t.enter(4, b3, p3);
  t.enter(5, b3, p3);
  return t;
}

// F14b : D1 = {0,1,2}, D2 = {3,4,5} (gros a mcs 3), petit {6} ; fusion N-aire au rayon 3 ; groupe lointain {7,8,9}
// et racine au rayon 10. Feuilles : D1 | D2 | {7,8,9}.
Tree f14b() {
  Tree t(10);
  const u32 p1 = t.radius(1), p3 = t.radius(3), p10 = t.radius(10);
  const u32 b0 = t.block(p1), b1 = t.block(p1), b2 = t.block(p1), b3 = t.block(p3, {b0, b1, b2}), b4 = t.block(p1);
  const u32 b5 = t.block(p10, {b3, b4});
  static_cast<void>(b5);
  for (u32 s : {0u, 1u, 2u}) t.enter(s, b0, p1);
  for (u32 s : {3u, 4u, 5u}) t.enter(s, b1, p1);
  t.enter(6, b2, p1);
  for (u32 s : {7u, 8u, 9u}) t.enter(s, b4, p1);
  return t;
}

// F14c : bloc {0,1} (rayon 1), le site 2 entre a la date sqrt 4 + sqrt 9 - sqrt 1 = 4, entre 3 et 5 ; groupe {3,4,5}
// au rayon 3 ; racine au rayon 9 ; mcs 3 : {0,1,2} | {3,4,5}.
Tree f14c() {
  Tree t(6);
  const u32 p1 = t.radius(1), p3 = t.radius(3), pd = t.date(frac(4), frac(9), frac(1)), p9 = t.radius(9);
  const u32 b0 = t.block(p1), b1 = t.block(p3), b2 = t.block(p9, {b0, b1});
  static_cast<void>(b2);
  t.enter(0, b0, p1);
  t.enter(1, b0, p1);
  t.enter(2, b0, pd);
  for (u32 s : {3u, 4u, 5u}) t.enter(s, b1, p3);
  return t;
}

// F14d : n < mcs, tout bruit.
Tree f14d() {
  Tree t(3);
  const u32 p1 = t.radius(1), p2 = t.radius(2);
  const u32 b0 = t.block(p1), b1 = t.block(p1), b2 = t.block(p2, {b0, b1});
  static_cast<void>(b2);
  t.enter(0, b0, p1);
  t.enter(1, b0, p1);
  t.enter(2, b1, p1);
  return t;
}

// F14e : deux arbres jamais reunis, {0,1,2} et {3,4,5}, mcs 3 : enfants de la racine virtuelle, admissibles.
Tree f14e() {
  Tree t(6);
  const u32 p1 = t.radius(1), p2 = t.radius(2);
  const u32 b0 = t.block(p1), b1 = t.block(p2);
  for (u32 s : {0u, 1u, 2u}) t.enter(s, b0, p1);
  for (u32 s : {3u, 4u, 5u}) t.enter(s, b1, p2);
  return t;
}

// Egalite EOM : D1 = {0,1}, D2 = {2,3} rejoints au niveau la, fusion P au niveau lb, Q = {4,5} rejoint a la ; P et Q
// fusionnent a lc (racine). S(D1) + S(D2) = 4 (phi(a) - phi(b)), S(P) = 4 (phi(b) - phi(c)).
Tree tie(const Rational& la, const Rational& lb, const Rational& lc) {
  Tree t(6);
  const u32 pa = t.square(la), pb = t.square(lb), pc = t.square(lc);
  const u32 b0 = t.block(pa), b1 = t.block(pa), b2 = t.block(pb, {b0, b1}), b3 = t.block(pa), b4 = t.block(pc, {b2, b3});
  static_cast<void>(b4);
  t.enter(0, b0, pa);
  t.enter(1, b0, pa);
  t.enter(2, b1, pa);
  t.enter(3, b1, pa);
  t.enter(4, b3, pa);
  t.enter(5, b3, pa);
  return t;
}

// Signe exact de phi(date) - phi(niveau l) a z.
int date_against(const Rational& t, const Rational& m, const Rational& q, const Rational& l, u32 z, Outcome& status) {
  Tree tree(0);
  const u32 pd = tree.date(t, m, q), ps = tree.square(l);
  const head::detail::Weight weights[2] = {{pd, 1}, {ps, -1}};
  MemoryBudget budget(MemoryBudget::kUnlimited);
  int sign = 9;
  status = head::detail::exact_sign(tree.view(), tree.levels, z, weights, budget, sign);
  return sign;
}

}  // namespace

MHGP11_TEST(fixtures, 7) {
  CHECK(same_groups(run(f14a(), 2, 1, head::Selection::eom), Groups{{0, 1}, {2, 3}, {4, 5}}));
  CHECK(same_groups(run(f14b(), 3, 1, head::Selection::leaves), Groups{{0, 1, 2}, {3, 4, 5}, {7, 8, 9}}));
  CHECK(same_groups(run(f14c(), 3, 1, head::Selection::eom), Groups{{0, 1, 2}, {3, 4, 5}}));
  const auto d = run(f14d(), 4, 1, head::Selection::eom);
  CHECK(d.ok() && groups(d.value().labels.span()).empty() && d.value().stats.noise == 3);
  for (const auto selection : {head::Selection::eom, head::Selection::leaves})
    CHECK(same_groups(run(f14e(), 3, 1, selection), Groups{{0, 1, 2}, {3, 4, 5}}));
  // L'ecart de 2^-70 n'est pas separe par l'encadrement : repli exact, sans egalite.
  const auto a = run(f14a(), 2, 1, head::Selection::eom);
  CHECK(a.ok() && a.value().stats.exact >= 1 && a.value().stats.equalities == 0);
}

MHGP11_TEST(equalities, 8) {
  // z = 1, rationnelle : 1/1 + 1/3 = 2 / (3/2) : egalite certifiee, le parent {0..3} l'emporte.
  const Tree rational = tie(frac(1), frac(9, 4), frac(9));
  const auto r1 = run(rational, 2, 1, head::Selection::eom);
  CHECK(same_groups(r1, Groups{{0, 1, 2, 3}, {4, 5}}));
  CHECK(r1.ok() && r1.value().stats.equalities == 1 && r1.value().stats.exact >= 1);
  // z = 1, irrationnelle : niveaux 2, 32/9, 8 (classe de sqrt 2).
  const auto i1 = run(tie(frac(2), frac(32, 9), frac(8)), 2, 1, head::Selection::eom);
  CHECK(same_groups(i1, Groups{{0, 1, 2, 3}, {4, 5}}));
  CHECK(i1.ok() && i1.value().stats.equalities == 1);
  // z = 2 : rayons 1, 7/5, 7 (1 + 1/49 = 2 / (49/25)) : egalite a z = 2, le parent l'emporte.
  const Tree square = tie(frac(1), frac(49, 25), frac(49));
  const auto s2 = run(square, 2, 2, head::Selection::eom);
  CHECK(same_groups(s2, Groups{{0, 1, 2, 3}, {4, 5}}));
  CHECK(s2.ok() && s2.value().stats.equalities == 1);
  // La meme geometrie a z = 1 n'est pas une egalite : 4 (2/b - 1/a - 1/c) = 8/7 > 0, parent sans egalite.
  const auto s1 = run(square, 2, 1, head::Selection::eom);
  CHECK(same_groups(s1, Groups{{0, 1, 2, 3}, {4, 5}}) && s1.value().stats.equalities == 0);
  // Et l'egalite de z = 1 n'en est pas une a z = 2 : enfants 4 (1 - 4/9) = 20/9 > parent 4 (4/9 - 1/9) = 12/9, les
  // enfants l'emportent.
  const auto r2 = run(rational, 2, 2, head::Selection::eom);
  CHECK(same_groups(r2, Groups{{0, 1}, {2, 3}, {4, 5}}) && r2.value().stats.equalities == 0);
}

// Egalite rationnelle a l'echelle 2^62 : racines R = floor(2^64 sqrt(l)) jusqu'a 3 2^126, hors de i128 une fois
// sommees (audit 100fcc12b) : plateaux sans encadrement, repli exact, egalite certifiee.
MHGP11_TEST(huge, 6) {
  auto scaled = [](i64 a, i64 b) {
    Big n, d;
    static_cast<void>(num::shift_left(Big::from_i64(a), 124, n));
    d = Big::from_i64(b);
    Rational out;
    static_cast<void>(Rational::make(n, d, out));
    return out;
  };
  const Tree t = tie(scaled(1, 1), scaled(9, 4), scaled(9, 1));
  const auto got = run(t, 2, 1, head::Selection::eom);
  CHECK(same_groups(got, Groups{{0, 1, 2, 3}, {4, 5}}));
  CHECK(got.ok() && got.value().stats.equalities == 1);
  // Racines 2^126, 1,5 2^126 et 3 2^126 : les trois plateaux depassent 2^100 ; sans la garde, seul le dernier (converti
  // en i128 negatif) serait ecarte, les deux autres encadres hors du domaine prouve.
  CHECK(got.ok() && got.value().stats.unbracketed == 3);
  // Temoin de l'auditeur : racine exactement 2^127 - 1 (l = (2^127 - 1)^2 / 2^128), ou R + 1 deborderait i128 ;
  // joue sous UBSan par la configuration ASan+UBSan de la matrice.
  auto edge = [](i64 a, i64 b) {
    Big r, r2, n, d;
    static_cast<void>(num::shift_left(Big::from_u64(1), 127, r));
    static_cast<void>(num::subtract(r, Big::from_u64(1), r));
    static_cast<void>(num::multiply(r, r, r2));
    static_cast<void>(num::multiply(r2, Big::from_i64(a), n));
    static_cast<void>(num::shift_left(Big::from_i64(b), 128, d));
    Rational out;
    static_cast<void>(Rational::make(n, d, out));
    return out;
  };
  // Rayons A/3, A/2 et A (A = (2^127 - 1) / 2^64) : 3/A + 1/A = 2 / (A/2), egalite certifiee.
  const Tree e = tie(edge(1, 9), edge(1, 4), edge(1, 1));
  u128 root = 0;
  CHECK(e.levels.root(2, root).ok() && root == (u128{1} << 127) - 1);
  const auto at_edge = run(e, 2, 1, head::Selection::eom);
  CHECK(same_groups(at_edge, Groups{{0, 1, 2, 3}, {4, 5}}));
  CHECK(at_edge.ok() && at_edge.value().stats.equalities == 1 && at_edge.value().stats.unbracketed == 3);
}

MHGP11_TEST(dates, 25) {
  Outcome status;
  for (u32 z = 1; z <= 3; ++z) {
    CHECK_EQ(date_against(frac(4), frac(9), frac(4), frac(9), z, status), 0);  // e = 3, Delta != 0
    CHECK(status.ok());
    CHECK_EQ(date_against(frac(8), frac(2), frac(8), frac(2), z, status), 0);  // e = sqrt 2, Delta != 0
    CHECK(status.ok());
    CHECK_EQ(date_against(frac(9), frac(4), frac(1), frac(16), z, status), 0);  // Delta = 0, e = 4
    CHECK(status.ok());
    // e = sqrt 2 + sqrt 3 - 1 = 2.146..., e^2 = 4.6063... : phi(date) < phi(sqrt 4.6), > phi(sqrt 4.61).
    CHECK_EQ(date_against(frac(2), frac(3), frac(1), frac(46, 10), z, status), -1);
    CHECK_EQ(date_against(frac(2), frac(3), frac(1), frac(461, 100), z, status), 1);
  }
  // Date non positive : sqrt 1 + sqrt 1 - sqrt 9 < 0.
  date_against(frac(1), frac(1), frac(9), frac(16), 1, status);
  CHECK_EQ(status.reason, Reason::head_invariant);
  // Delta = 0 avec e = 0 : q = (sqrt t + sqrt m)^2.
  date_against(frac(1), frac(4), frac(9), frac(16), 1, status);
  CHECK_EQ(status.reason, Reason::head_invariant);
}

MHGP11_TEST(order, 6) {
  Tree t = f14c();
  const u32 ids[6] = {907, 13, 4000000000u, 5, 77, 600};
  for (u32 s = 0; s < 6; ++s) t.ids[s] = make_id<PointId>(ids[s]);
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto sites = head::flat_sites(t.view(), t.levels, {3, 1, head::Selection::eom}, budget);
  REQUIRE(sites.ok());
  // Ordre d'entree : permutation des sites.
  const std::vector<PointId> input = {t.ids[4], t.ids[0], t.ids[5], t.ids[2], t.ids[3], t.ids[1]};
  auto ordered = head::in_input_order(sites.value().labels.span(), t.ids, input, budget);
  REQUIRE(ordered.ok());
  const std::vector<i64> expected = {5, 13, 5, 13, 5, 13};  // {0,1,2} -> min(907, 13, 4e9) ; {3,4,5} -> 5
  CHECK(std::vector<i64>(ordered.value().begin(), ordered.value().end()) == expected);
  std::vector<PointId> missing = input;
  missing[2] = make_id<PointId>(1);
  CHECK_EQ(head::in_input_order(sites.value().labels.span(), t.ids, missing, budget).outcome().reason,
           Reason::head_invariant);
  std::vector<PointId> twice = input;
  twice[2] = input[0];
  CHECK_EQ(head::in_input_order(sites.value().labels.span(), t.ids, twice, budget).outcome().reason,
           Reason::head_invariant);
  CHECK_EQ(head::in_input_order(sites.value().labels.span(), t.ids, std::span<const PointId>(input).first(5), budget)
               .outcome()
               .reason,
           Reason::head_invariant);
  CHECK_EQ(ordered.value().size(), 6u);
}

MHGP11_TEST(refusals, 14) {
  const Tree t = f14c();
  CHECK_EQ(run(t, 1, 1, head::Selection::eom).outcome().reason, Reason::parameter_out_of_range);
  CHECK_EQ(run(t, 3, 0, head::Selection::eom).outcome().reason, Reason::parameter_out_of_range);
  CHECK_EQ(run(t, 3, 4, head::Selection::eom).outcome().reason, Reason::parameter_out_of_range);
  // Jonction au niveau nul : {0,1,2} entre au niveau 0 (gros a mcs 2), puis fusion.
  Tree zero(4);
  const u32 p0 = zero.square(frac(0)), p1 = zero.radius(1), p2 = zero.radius(2);
  const u32 b0 = zero.block(p0), b1 = zero.block(p1), b2 = zero.block(p2, {b0, b1});
  static_cast<void>(b2);
  zero.enter(0, b0, p0);
  zero.enter(1, b0, p0);
  zero.enter(2, b1, p1);
  zero.enter(3, b1, p1);
  CHECK_EQ(run(zero, 2, 1, head::Selection::eom).outcome().reason, Reason::head_invariant);
  // Feuilles : aucun score, le niveau nul n'est pas lu.
  CHECK(run(zero, 2, 1, head::Selection::leaves).ok());
  // Arbre mal forme : parent anterieur.
  Tree bad = f14c();
  bad.block_parent[1] = 0;
  CHECK_EQ(run(bad, 3, 1, head::Selection::eom).outcome().reason, Reason::head_invariant);
  // Chronologie des entrees : naissance du bloc <= entree < naissance du parent ; racine sans borne haute.
  // condense seul isole la forme, sans score ni appel de phi(0).
  auto chronology = [](u32 block, u32 plateau) {
    Tree tree(3);
    const u32 p0 = tree.radius(1), p1 = tree.radius(2), p2 = tree.radius(3);
    static_cast<void>(p2);
    const u32 b0 = tree.block(p0), b1 = tree.block(p0), b2 = tree.block(p1, {b0, b1});
    static_cast<void>(b2);
    tree.enter(0, b0, p0);
    tree.enter(1, b1, p0);
    tree.enter(2, block, plateau);
    MemoryBudget budget(MemoryBudget::kUnlimited);
    Outcome status;
    {
      head::detail::Condensed out;
      status = head::detail::condense(tree.view(), 2, budget, out);
    }
    return std::pair{status, budget.used()};
  };
  const auto before = chronology(0, 0), equal = chronology(0, 1), after = chronology(0, 2);
  const auto root_birth = chronology(2, 1), root_later = chronology(2, 2), not_born = chronology(2, 0);
  CHECK(before.first.ok() && before.second == 0);
  CHECK(equal.first.reason == Reason::head_invariant && equal.second == 0);
  CHECK(after.first.reason == Reason::head_invariant && after.second == 0);
  CHECK(root_birth.first.ok() && root_birth.second == 0);
  CHECK(root_later.first.ok() && root_later.second == 0);
  CHECK(not_born.first.reason == Reason::head_invariant && not_born.second == 0);
  // Budget trop petit : refus, budget rendu.
  MemoryBudget small(256);
  CHECK_EQ(head::flat_sites(t.view(), t.levels, {3, 1, head::Selection::eom}, small).outcome().reason,
           Reason::memory_budget);
  CHECK_EQ(small.used(), 0u);
}

MHGP11_TEST(budget, 2) {
  const Tree t = f14a();
  MemoryBudget budget(MemoryBudget::kUnlimited);
  {
    auto sites = head::flat_sites(t.view(), t.levels, {2, 1, head::Selection::eom}, budget);
    REQUIRE(sites.ok());
    CHECK_EQ(budget.used(), sites.value().labels.size() * sizeof(i64));  // seul le produit reste reserve
  }
  CHECK_EQ(budget.used(), 0u);
}

MHGP11_TEST_MAIN()
