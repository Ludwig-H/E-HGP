// Portes unitaires de l'etage G (CONTRAT_TOUR.md, paragraphes 4, 7 et 9) : cibles de 4 octets et capacite, temoins
// exacts (WIT-D2, WIT-MEMO, WIT-T1-CARRE cote tour, faits fact_complete_census_in_catalogue et
// fact_saturated_in_catalogue de reference/test_resolution_v12.py, pas inerte sous la fenetre), plafonds declares
// (cell_capacity ; WIT-SPHERE50 refuse par le catalogue, shell_capacity), budget, determinisme a 1 et 8 fils. Les
// attendus des temoins sont ceux de resolve_v12 (reference/hgp12_ref, politique v12_indices), graves ici.
#include "unit_support.hpp"

using namespace tower_test;

namespace {

// Cible de la partie f de l'ordre k, rendue comme boule (BallIdx) et genre ; kNone si absente.
struct Target {
  bool cell = false;
  u32 ball = kNone;
};
Target decode(const Case& c, Order k, u32 target) {
  const ResolvedOrder& o = c.resolution->order(k);
  if (target_is_cell(target)) return {true, idx(o.cell_balls()[target_index(target)])};
  return {false, o.birth_keys()[target]};
}

// Rang de la cible d'un representant (naissance ou cellule) ; date de lecture : strictement sous la jonction.
u32 target_rank(const ResolvedOrder& o, u32 target) {
  return target_is_cell(target) ? idx(o.cell_ranks()[target_index(target)]) : idx(o.birth_ranks()[target]);
}

std::vector<Xyz> sphere50_pairs(u32 pairs) {
  const std::vector<std::array<int, 3>> base{{0, 1, -7}, {0, 1, 7}, {0, 5, -5}, {0, 5, 5}, {0, 7, -1}, {0, 7, 1},
                                             {1, -7, 0}, {1, 0, -7}, {1, 0, 7}, {1, 7, 0}, {3, -5, -4}, {3, -5, 4},
                                             {3, -4, -5}, {3, -4, 5}, {3, 4, -5}};
  std::vector<Xyz> out;
  for (u32 i = 0; i < pairs && i < base.size(); ++i)
    for (int s : {1, -1})
      out.push_back({static_cast<u32>(100 + s * base[i][0]), static_cast<u32>(100 + s * base[i][1]),
                     static_cast<u32>(100 + s * base[i][2])});
  return out;
}

}  // namespace

// Cibles de 4 octets : genre au bit 31, 31 bits d'indice, 0xFFFFFFFF jamais produit ; capacite d'un ordre avant toute
// allocation : 2^31 - 1 naissances ou cellules admises, 2^31 refusees ; 2^32 - 1 representants admis, 2^32 refuses.
MHGP12_TEST(targets_capacity, 16) {
  CHECK_EQ(birth_target(0), 0u);
  CHECK_EQ(cell_target(0), 0x80000000u);
  CHECK(target_is_cell(cell_target(5)) && !target_is_cell(birth_target(5)));
  CHECK_EQ(target_index(cell_target(0x7FFFFFFEu)), 0x7FFFFFFEu);
  CHECK(cell_target(static_cast<u32>(kMaxOrderCells - 1)) != kNoTarget);  // plus grand indice : 2^31 - 2
  CHECK(birth_target(static_cast<u32>(kMaxOrderBirths - 1)) != kNoTarget);
  CHECK(check_order_capacity(kMaxOrderBirths, kMaxOrderCells, kMaxOrderRepresentatives).ok());
  CHECK_EQ(check_order_capacity(kMaxOrderBirths + 1, 0, 0).reason, Reason::tower_capacity);
  CHECK_EQ(check_order_capacity(0, kMaxOrderCells + 1, 0).reason, Reason::tower_capacity);
  CHECK_EQ(check_order_capacity(0, 0, kMaxOrderRepresentatives + 1).reason, Reason::tower_capacity);
  CHECK_EQ(check_order_capacity(u64{1} << 32, 0, 0).reason, Reason::tower_capacity);
  CHECK(check_order_capacity(0, 0, 0).ok());
  CHECK_EQ(status_of(Reason::tower_capacity), Status::resource_exhausted);
  CHECK_EQ(status_of(Reason::cell_capacity), Status::unsupported_degeneracy);
  CHECK_EQ(status_of(Reason::catalogue_missing_ball), Status::invariant_violated);
  CHECK_EQ(status_of(Reason::census_mismatch), Status::invariant_violated);
}

// WIT-D2 (K = 2) : la boule faible ABC (niveau 1681/25) a la trace stricte AB, de plus petite boule hors de Cat_2
// (niveau 64, Z et W interieurs), nee APRES le niveau precedent du catalogue (41) : aucune garde beta(F0) <= l(r_b-1)
// (CST-0104). Route exacte : certificat, census sature de seuil 2, saut vers {Z, W}, naissance ZW par la table.
MHGP12_TEST(witness_d2, 15) {
  const Xyz a{2, 10, 0}, b{18, 10, 0}, cc{10, 20, 0}, z{9, 3, 0}, w{11, 3, 0};
  auto c = build({a, b, cc, z, w}, 2);
  REQUIRE(c->outcome.ok());
  const u32 abc = c->ball({a, b, cc}), zw = c->ball({z, w});
  REQUIRE(abc != kNone && zw != kNone);
  const ResolvedOrder& o = c->resolution->order(2);
  const u32 cell = target_index(c->resolution->window_target(make_id<BallIdx>(abc), 2));
  REQUIRE(target_is_cell(c->resolution->window_target(make_id<BallIdx>(abc), 2)));
  CHECK_EQ(o.cell_offsets()[cell + 1] - o.cell_offsets()[cell], 3u);  // AB, AC, BC : jonction reguliere
  CHECK_EQ(o.cell_flags()[cell], 0u);
  auto direct = resolve_direct(*c, c->part({a, b}), 2, c->rank(abc));
  REQUIRE(direct.target.ok());
  const Target t = decode(*c, 2, direct.target.value());
  CHECK(!t.cell);
  CHECK_EQ(t.ball, zw);
  CHECK_EQ(direct.counters.route_cert_census, 1u);
  CHECK_EQ(direct.counters.census_saturated, 1u);
  CHECK_EQ(direct.counters.jumps_census, 1u);
  CHECK_EQ(direct.counters.probe_hits_after_steps, 1u);
  CHECK_EQ(direct.counters.controls, 2u);  // la plus petite boule de AB, puis la naissance ZW
  u64 mask = 0;  // A = {a, b} dans U de ABC (bit j = j-ieme site de U)
  const auto shell = c->catalogue->shell(make_id<BallIdx>(abc));
  for (u32 j = 0; j < shell.size(); ++j)
    if (idx(shell[j]) == c->site(a) || idx(shell[j]) == c->site(b)) mask |= u64{1} << j;
  bool found = false;  // la meme cible dans la sortie de l'etage, trace AB de la cellule ABC
  for (u64 r = o.cell_offsets()[cell]; r < o.cell_offsets()[cell + 1]; ++r)
    if (o.trace_masks()[r] == mask) found = o.targets()[r] == direct.target.value();
  CHECK(found);
  CHECK(c->resolution->order(1).representatives() > 0);
}

// WIT-MEMO (ligne {0, 2, 4, 6}, K = 2 et 3) : une cible ne vaut qu'a partir de sa date. Toute cible a un rang
// strictement inferieur a celui de sa jonction ; une partie resolue sous une jonction de niveau inferieur ou egal a sa
// plus petite boule est refusee (date initiale controlee) : {0, 6} (niveau 9) sous la jonction [0, 4] (niveau 4).
MHGP12_TEST(witness_memo, 12) {
  const std::vector<Xyz> line{{0, 0, 0}, {2, 0, 0}, {4, 0, 0}, {6, 0, 0}};
  for (int kmax : {2, 3}) {
    auto c = build(line, kmax);
    REQUIRE(c->outcome.ok());
    u64 dated = 0, late = 0;
    for (Order k = 2; k <= c->resolution->orders(); ++k) {
      const ResolvedOrder& o = c->resolution->order(k);
      for (u32 cell = 0; cell < o.cells(); ++cell)
        for (u64 r = o.cell_offsets()[cell]; r < o.cell_offsets()[cell + 1]; ++r) {
          ++dated;
          late += target_rank(o, o.targets()[r]) >= idx(o.cell_ranks()[cell]);
        }
    }
    CHECK(dated >= 2);
    CHECK_EQ(late, 0u);
    const u32 j04 = c->ball({{0, 0, 0}, {2, 0, 0}, {4, 0, 0}});
    REQUIRE(j04 != kNone);
    auto early = resolve_direct(*c, c->part({{0, 0, 0}, {6, 0, 0}}), 2, c->rank(j04));
    CHECK_EQ(early.target.outcome().reason, Reason::tower_invariant);
    CHECK_EQ(early.target.outcome().order, 2u);
  }
}

// WIT-T1-CARRE cote tour (carre ABCD et point lointain E, K = 5) : LEM-T1 exige S dans F ; la diagonale BD, support
// minimal non canonique, n'est pas dans la table ; F = BD passe par la route exacte (certificat puis census COMPLET a
// l'ordre 2 = K - 3 : fact_complete_census_in_catalogue) et s'arrete sur la cellule (cercle, 2).
MHGP12_TEST(witness_t1_square, 16) {
  const Xyz a{0, 0, 0}, b{2, 0, 0}, cc{2, 2, 0}, d{0, 2, 0}, e{40, 40, 40};
  auto c = build({a, b, cc, d, e}, 5);
  REQUIRE(c->outcome.ok());
  const auto domain = c->domain();
  const u32 circle = c->ball({a, b, cc, d}), far = c->ball({cc, e});
  REQUIRE(circle != kNone && far != kNone);
  const auto ac = c->part({a, cc}), ab = c->part({a, b}), bd = c->part({b, d});
  const std::span<const u32> s_ac(ac.id.data(), 2), s_bd(bd.id.data(), 2);
  CHECK(!tower_detail::lem_t1(domain, ab, s_ac).has_value());  // S = S*(cercle) hors de F : jamais un succes
  CHECK(tower_detail::lem_t1(domain, ac, s_ac) == std::optional<u32>(circle));
  CHECK(!tower_detail::lem_t1(domain, bd, s_bd).has_value());  // support non canonique : table en echec
  auto direct = resolve_direct(*c, bd, 2, c->rank(far));
  REQUIRE(direct.target.ok());
  const Target t = decode(*c, 2, direct.target.value());
  CHECK(t.cell);
  CHECK_EQ(t.ball, circle);
  CHECK_EQ(direct.counters.route_cert_census, 1u);
  CHECK_EQ(direct.counters.census_complete, 1u);
  CHECK_EQ(direct.counters.cell_stops, 1u);
  CHECK_EQ(direct.counters.route_t1, 0u);
  auto side = resolve_direct(*c, ab, 2, c->rank(circle));  // un cote : sa propre naissance, a la premiere sonde
  REQUIRE(side.target.ok());
  CHECK_EQ(decode(*c, 2, side.target.value()).ball, c->ball({a, b}));
  CHECK_EQ(side.counters.first_probe_hits, 1u);
  CHECK_EQ(c->resolution->order(2).counters().inert_cells, 0u);
}

// fact_saturated_in_catalogue (carre (0,0,0), (4,0,0), (4,4,0), (0,4,0), interieurs (2,2,0) et (2,1,0), K = 5 ; un
// point lointain (40,40,40) date la jonction) : la diagonale (4,0,0)-(0,4,0) a pour boule une sphere du catalogue
// (p = 2) dont S* est l'autre diagonale ; la table echoue, le census de seuil 2 sature (il prouve seulement p >= k),
// la decroissance est controlee AVANT le saut, puis naissance de {(2,1,0), (2,2,0)} par la table.
MHGP12_TEST(fact_saturated, 10) {
  const Xyz a{0, 0, 0}, b{4, 0, 0}, cc{4, 4, 0}, d{0, 4, 0}, i1{2, 2, 0}, i2{2, 1, 0}, e{40, 40, 40};
  auto c = build({a, b, cc, d, i1, i2, e}, 5);
  REQUIRE(c->outcome.ok());
  const u32 big = c->ball({a, b, cc, d, i1, i2}), inner = c->ball({i1, i2}), far = c->ball({cc, e});
  REQUIRE(big != kNone && inner != kNone && far != kNone);
  CHECK_EQ(c->catalogue->balls_data()[big].p, 2u);
  CHECK(idx(c->rank(big)) < idx(c->rank(far)));
  auto direct = resolve_direct(*c, c->part({b, d}), 2, c->rank(far));
  REQUIRE(direct.target.ok());
  const Target t = decode(*c, 2, direct.target.value());
  CHECK(!t.cell);
  CHECK_EQ(t.ball, inner);
  CHECK_EQ(direct.counters.census_saturated, 1u);
  CHECK_EQ(direct.counters.jumps_census, 1u);
  CHECK_EQ(direct.counters.controls, direct.counters.smallest_balls() + direct.counters.first_probe_hits +
                                         direct.counters.probe_hits_after_steps);
  CHECK_EQ(direct.counters.controls, 2u);
}

// Pas inerte sous la fenetre (triangle aigu (0,0,0), (10,0,0), (5,8,0), interieurs (5,2,0) et (5,3,0), point
// lointain (60,60,60), K = 4) : la boule du triangle (p = 2, q_min = 3) a la fenetre [4, 4] ; le triangle a l'ordre 3
// fait un pas inerte vers I u {(0,0,0)} (premier site de U), population de la boule (0,0,0)-(5,3,0) : naissance par
// la table. S'arreter sur la boule du triangle (hors de sa fenetre) serait faux.
MHGP12_TEST(inert_below_window, 9) {
  const Xyz a{0, 0, 0}, b{10, 0, 0}, cc{5, 8, 0}, z{5, 2, 0}, w{5, 3, 0}, e{60, 60, 60};
  auto c = build({a, b, cc, z, w, e}, 4);
  REQUIRE(c->outcome.ok());
  const u32 tri = c->ball({a, b, cc, z, w}), next = c->ball({a, z, w}), far = c->ball({cc, e});
  REQUIRE(tri != kNone && next != kNone && far != kNone);
  CHECK(idx(c->rank(tri)) < idx(c->rank(far)));
  CHECK_EQ(c->resolution->window_target(make_id<BallIdx>(tri), 3), kNoTarget);  // hors de la fenetre
  auto direct = resolve_direct(*c, c->part({a, b, cc}), 3, c->rank(far));
  REQUIRE(direct.target.ok());
  const Target t = decode(*c, 3, direct.target.value());
  CHECK(!t.cell);
  CHECK_EQ(t.ball, next);
  CHECK_EQ(direct.counters.inert_steps, 1u);
  CHECK_EQ(direct.counters.probe_hits_after_steps, 1u);
}

// Plafonds declares : 30 sites cospheriques (15 paires antipodales de x^2 + y^2 + z^2 = 50, K = 5) : C(30, 4) = 27 405
// t-parties a l'ordre 4, au-dela de kMaxCellCombinations : refus cell_capacity, rien de publie. WIT-SPHERE50 (84
// sites de la sphere) est refuse plus tot par le catalogue (coquille de plus de 64 sites, shell_capacity) : la tour ne
// recoit jamais une coquille plus large que ses masques.
MHGP12_TEST(capacity_refusals, 6) {
  auto c = build(sphere50_pairs(15), 5);
  CHECK_EQ(c->outcome.reason, Reason::cell_capacity);
  CHECK(!c->resolution.has_value());
  CHECK(c->catalogue.has_value());
  std::vector<Xyz> all;
  for (int x = -8; x <= 8; ++x)
    for (int y = -8; y <= 8; ++y)
      for (int z = -8; z <= 8; ++z)
        if (x * x + y * y + z * z == 50)
          all.push_back({static_cast<u32>(100 + x), static_cast<u32>(100 + y), static_cast<u32>(100 + z)});
  CHECK_EQ(all.size(), 84u);
  auto sphere = build(all, 5);
  CHECK_EQ(sphere->outcome.reason, Reason::shell_capacity);
  CHECK(!sphere->catalogue.has_value());
}

// Budget : un budget trop petit pour les sorties refuse memory_budget sans rien publier ; apres destruction du
// resultat, le budget revient a ce qu'il etait avant l'appel (aucune reservation perdue).
MHGP12_TEST(budget, 5) {
  auto c = build({{0, 0, 0}, {3, 1, 0}, {1, 4, 2}, {5, 5, 5}, {2, 7, 1}, {6, 0, 3}}, 3);
  REQUIRE(c->outcome.ok());
  const u64 before = c->budget.used();
  {
    auto again = resolve_tower(*c->index, *c->catalogue, c->budget, *c->pool);
    CHECK(again.ok());
    CHECK(c->budget.used() > before);
  }
  CHECK_EQ(c->budget.used(), before);
  MemoryBudget tight(1024);  // budget de l'etage seul : l'index et le catalogue sont empruntes
  auto refused = resolve_tower(*c->index, *c->catalogue, tight, *c->pool);
  CHECK_EQ(refused.outcome().reason, Reason::memory_budget);
  CHECK(tight.released().ok());
}

// Determinisme : nuage uniforme de 2 000 sites (SplitMix64), K = 5, a 1 et 8 fils : memes naissances, cellules,
// traces, cibles et compteurs (objet et travail), ordre par ordre.
MHGP12_TEST(determinism, 60) {
  std::vector<Xyz> pts;
  u64 state = 20261007;
  auto next = [&state]() {
    state += 0x9E3779B97F4A7C15ull;
    u64 z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return static_cast<u32>((z ^ (z >> 31)) & 0xFFFF);
  };
  for (int i = 0; i < 2000; ++i) pts.push_back({next(), next(), next()});
  std::sort(pts.begin(), pts.end());
  pts.erase(std::unique(pts.begin(), pts.end()), pts.end());
  auto one = build(pts, 5, 1), eight = build(pts, 5, 8);
  REQUIRE(one->outcome.ok() && eight->outcome.ok());
  const Resolution &r1 = *one->resolution, &r8 = *eight->resolution;
  REQUIRE(r1.orders() == 5 && r8.orders() == 5);
  for (Order k = 1; k <= 5; ++k) {
    const ResolvedOrder &a = r1.order(k), &b = r8.order(k);
    auto same = [](auto x, auto y) { return std::equal(x.begin(), x.end(), y.begin(), y.end()); };
    CHECK(same(a.birth_keys(), b.birth_keys()));
    CHECK(same(a.birth_ranks(), b.birth_ranks()));
    CHECK(same(a.cell_balls(), b.cell_balls()));
    CHECK(same(a.cell_flags(), b.cell_flags()));
    CHECK(same(a.cell_offsets(), b.cell_offsets()));
    CHECK(same(a.trace_masks(), b.trace_masks()));
    CHECK(same(a.targets(), b.targets()));
    CHECK(a.counters() == b.counters());
    CHECK(a.representatives() > 0);
    CHECK(k == 1 || a.counters().census_saturated > 0);
    CHECK(k == 1 || a.counters().controls > 0);
    CHECK(k == 1 || a.counters().cell_stops > 0);
  }
}

MHGP12_TEST_MAIN()
