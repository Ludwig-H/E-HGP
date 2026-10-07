// Portes unitaires du catalogue (tranche T1, CONTRAT_CATALOGUE.md, paragraphe 6) : temoins exacts graves, refus,
// paliers, determinisme. Les attendus << v11 >> sont ceux de la v11 gelee (ac081a06f, sonde de travail sur
// build_catalogue au masque 802811 : feuille donnee, max_leaf 256, cache J2, graphe de paires), graves ici.
#include <algorithm>
#include <array>
#include <memory>
#include <string_view>
#include <vector>

#include "catalogue/internal.hpp"
#include "sched/sched.hpp"
#include "test.hpp"

using namespace mhgp12;

namespace {

using Points = std::vector<std::array<u32, 3>>;

Result<Cloud> make_cloud(const Points& pts, MemoryBudget& budget) {
  std::vector<u32> x, y, z;
  std::vector<PointId> ids;
  for (std::size_t i = 0; i < pts.size(); ++i) {
    x.push_back(pts[i][0]);
    y.push_back(pts[i][1]);
    z.push_back(pts[i][2]);
    ids.push_back(make_id<PointId>(static_cast<u32>(i)));
  }
  return prepare_cloud(x, y, z, ids, CoordWidth(), budget);
}

Result<Catalogue> run(const Cloud& cloud, int k, u32 leaf, u32 threads, MemoryBudget& budget,
                      CatalogueDiagnostics* diag = nullptr, u32 max_leaf = kCatalogueMaxLeaf) {
  auto pool = sched::make_pool({threads});
  if (!pool.ok()) return pool.outcome();
  CatalogueParams p;
  p.kmax = k;
  p.leaf_size = leaf;
  p.max_leaf = max_leaf;
  return build_catalogue(cloud, p, budget, *pool.value(), diag);
}

// Coquille de la v12 (MES-M5, fixtures.py) : permutations signees de base, a l'echelle, autour de offset.
Points shell(std::array<u32, 3> base, u32 scale, u32 offset) {
  Points out;
  std::array<u32, 3> p = base;
  std::sort(p.begin(), p.end());
  do {
    for (int signs = 0; signs < 8; ++signs) {
      std::array<u32, 3> q{};
      for (int a = 0; a < 3; ++a) q[a] = ((signs >> a) & 1) ? offset + p[a] * scale : offset - p[a] * scale;
      out.push_back(q);
    }
  } while (std::next_permutation(p.begin(), p.end()));
  std::sort(out.begin(), out.end());
  out.erase(std::unique(out.begin(), out.end()), out.end());
  return out;
}

// Grand livre grave : 20 compteurs dans l'ordre de CatalogueLedger.
std::array<u64, 20> values(const CatalogueLedger& l) {
  return {l.nodes, l.leaves, l.filter_tests, l.dominance_tests, l.prefixes, l.judged, l.census_tests, l.emitted,
          l.incidences, l.q4_candidates, l.q4_levels, l.region_pair_tests, l.region_pair_rejects,
          l.region_line_tests, l.region_line_rejects, l.region_line_evaluations, l.region_line_cache_hits,
          l.region_line_fallbacks, l.max_leaf, l.max_depth};
}

std::optional<BallIdx> ball_with_shell(const Catalogue& c, u32 m, u32 p, u8 qmin) {
  for (u32 b = 0; b < c.balls(); ++b) {
    const auto& ball = c.balls_data()[b];
    if (ball.m == m && ball.p == p && ball.qmin == qmin) return make_id<BallIdx>(b);
  }
  return std::nullopt;
}

SiteIdx site_at(const Cloud& cloud, std::array<u32, 3> p) {
  for (u32 s = 0; s < cloud.sites(); ++s)
    if (cloud.x()[s] == p[0] && cloud.y()[s] == p[1] && cloud.z()[s] == p[2]) return make_id<SiteIdx>(s);
  return make_id<SiteIdx>(kNone);
}

std::array<SiteIdx, 2> pair(const Cloud& cloud, std::array<u32, 3> a, std::array<u32, 3> b) {
  SiteIdx sa = site_at(cloud, a), sb = site_at(cloud, b);
  if (idx(sb) < idx(sa)) std::swap(sa, sb);
  return {sa, sb};
}

}  // namespace

// WIT-T1-CARRE, cote catalogue : le carre et ses deux diagonales. La boule circonscrite a deux supports minimaux ; la
// table S* -> boule ne connait que S* (la diagonale de plus petite liste de positions), jamais l'autre diagonale ;
// un cote a sa propre boule.
MHGP12_TEST(witness_square, 12) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const Points square{{0, 0, 0}, {2, 0, 0}, {2, 2, 0}, {0, 2, 0}};
  auto cloud = make_cloud(square, budget);
  REQUIRE(cloud.ok());
  auto cat = run(cloud.value(), 2, 5, 2, budget);
  REQUIRE(cat.ok());
  const auto& c = cat.value();
  const auto big = ball_with_shell(c, 4, 0, 2);
  REQUIRE(big.has_value());
  const auto ac = pair(cloud.value(), {0, 0, 0}, {2, 2, 0}), bd = pair(cloud.value(), {2, 0, 0}, {0, 2, 0});
  const auto ab = pair(cloud.value(), {0, 0, 0}, {2, 0, 0});
  CHECK(c.balls_data()[idx(*big)].support[0] == ac[0]);
  CHECK(c.balls_data()[idx(*big)].support[1] == ac[1]);
  CHECK(c.find_support(ac) == big);
  CHECK(!c.find_support(bd).has_value());  // support minimal non canonique : recherche en echec, chemin exact
  const auto side = c.find_support(ab);
  REQUIRE(side.has_value());
  CHECK_EQ(c.balls_data()[idx(*side)].m, 2u);
  CHECK_EQ(c.balls_data()[idx(*side)].p, 0u);
  const std::array<SiteIdx, 2> reversed{ac[1], ac[0]};
  CHECK(!c.find_support(reversed).has_value());  // S* exige des SiteIdx croissants
  CHECK(!c.find_support(std::span<const SiteIdx>(ac.data(), 1)).has_value());
  CHECK_EQ(c.balls(), 5u);
  CHECK(budget.used() > 0);
}

// Convention de S* de la v12 (CST-0113) : rectangle (3,3,4), (3,4,3), (5,5,4), (5,4,5) de centre (4,4,4), deux
// diagonales. Le minimum des rangs de Morton (v11) est {(3,4,3), (5,4,5)} ; le minimum des positions (v12) est
// {(3,3,4), (5,5,4)}.
MHGP12_TEST(witness_rectangle, 6) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const Points rectangle{{3, 3, 4}, {3, 4, 3}, {5, 5, 4}, {5, 4, 5}};
  auto cloud = make_cloud(rectangle, budget);
  REQUIRE(cloud.ok());
  auto cat = run(cloud.value(), 1, 4, 1, budget);
  REQUIRE(cat.ok());
  const auto& c = cat.value();
  const auto big = ball_with_shell(c, 4, 0, 2);
  REQUIRE(big.has_value());
  const auto positions = pair(cloud.value(), {3, 3, 4}, {5, 5, 4}), morton = pair(cloud.value(), {3, 4, 3}, {5, 4, 5});
  CHECK(c.balls_data()[idx(*big)].support[0] == positions[0] && c.balls_data()[idx(*big)].support[1] == positions[1]);
  CHECK(c.find_support(positions) == big);
  CHECK(!c.find_support(morton).has_value());
  CHECK(idx(morton[0]) < idx(positions[0]));  // le temoin separe bien les deux conventions
}

// WIT-TRANSL (contre-lecture du contrat T1 de l'auditeur Codex) : le triangle A = (0,1,1), B = (1,0,1), C = (1,1,0)
// porte trois boules diametrales de niveau 1/2 a support unique ; Cat_1 les publie dans l'ordre des S* par positions,
// AB, AC, BC (la v11, par rangs de Morton, publiait BC, AC, AB), et une translation ne change pas cet ordre.
MHGP12_TEST(witness_transl, 8) {
  for (u32 shift : {0u, 1000u}) {
    MemoryBudget budget(MemoryBudget::kUnlimited);
    const std::array<u32, 3> a{shift, 1 + shift, 1 + shift}, b{1 + shift, shift, 1 + shift}, c{1 + shift, 1 + shift, shift};
    auto cloud = make_cloud({a, b, c}, budget);
    REQUIRE(cloud.ok());
    auto cat = run(cloud.value(), 1, 4, 1, budget);
    REQUIRE(cat.ok());
    const auto& balls = cat.value().balls_data();
    REQUIRE(balls.size() == 3u);
    const std::array<std::array<SiteIdx, 2>, 3> order{pair(cloud.value(), a, b), pair(cloud.value(), a, c),
                                                     pair(cloud.value(), b, c)};
    for (u32 i = 0; i < 3; ++i)
      CHECK(balls[i].support[0] == order[i][0] && balls[i].support[1] == order[i][1] && idx(balls[i].rank) == 1u);
    CHECK_EQ(cat.value().levels().size(), 2u);
  }
}

// Repli exact effectivement necessaire : le triangle equilateral (M,0,0), (0,M,0), (0,0,M), M = 2^B - 1, est une
// feuille d'etendue B + 1 (racine fermee a 2^B) ; dans la fabrique q3, t = |u|^2 v - |v|^2 u vaut 2 M^3 >= 2^63 et
// deborde l'i64 de la voie etroite. Cat_2 : trois boules diametrales et la boule du triangle (q_min = 3, m = 3, p = 0),
// jouees par le repli exact ; une feuille jouee sans elargir l'arithmetique perd ou fausse la boule du triangle.
MHGP12_TEST(witness_long_triangle, 6) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const u32 m = kCoordMax;
  auto cloud = make_cloud({{m, 0, 0}, {0, m, 0}, {0, 0, m}}, budget);
  REQUIRE(cloud.ok());
  CatalogueDiagnostics diag;
  auto cat = run(cloud.value(), 2, 5, 1, budget, &diag);
  REQUIRE(cat.ok());
  CHECK_EQ(cat.value().balls(), 4u);
  const auto triangle = ball_with_shell(cat.value(), 3, 0, 3);
  REQUIRE(triangle.has_value());
  CHECK_EQ(diag.leaves_exact, 1u);
  CHECK_EQ(diag.max_leaf_span, static_cast<u64>(kCoordBits + 1));
  CHECK(2.0 * double(m) * double(m) * double(m) >= 9223372036854775808.0);  // 2 M^3 >= 2^63
}

// WIT-SPHERE50 : les 84 sites entiers de x^2 + y^2 + z^2 = 50 (translates de (8,8,8)) forment une coquille de 84
// sites, au-dela du plafond declare de 64 : refus explicite shell_capacity (unsupported_degeneracy), rien de publie,
// rien de reserve apres le refus.
MHGP12_TEST(witness_sphere50, 4) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  Points sphere;
  for (int x = -8; x <= 8; ++x)
    for (int y = -8; y <= 8; ++y)
      for (int z = -8; z <= 8; ++z)
        if (x * x + y * y + z * z == 50)
          sphere.push_back({static_cast<u32>(x + 8), static_cast<u32>(y + 8), static_cast<u32>(z + 8)});
  CHECK_EQ(sphere.size(), 84u);
  auto cloud = make_cloud(sphere, budget);
  REQUIRE(cloud.ok());
  const u64 before = budget.used();
  auto cat = run(cloud.value(), 2, 8, 2, budget);
  CHECK(!cat.ok() && cat.outcome().reason == Reason::shell_capacity);
  CHECK(cat.outcome().status() == Status::unsupported_degeneracy);
  CHECK_EQ(budget.used(), before);
}

// WIT-FEUILLES : le cube {0,16}^3 et son centre, K5, feuille 8 : 80 feuilles pour 9 sites (<< feuilles <= sites >>
// est faux) ; grand livre complet egal a celui de la v11.
MHGP12_TEST(witness_leaves, 3) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  Points cube{{8, 8, 8}};
  for (u32 x : {0u, 16u})
    for (u32 y : {0u, 16u})
      for (u32 z : {0u, 16u}) cube.push_back({x, y, z});
  auto cloud = make_cloud(cube, budget);
  REQUIRE(cloud.ok());
  auto cat = run(cloud.value(), 5, 8, 2, budget);
  REQUIRE(cat.ok());
  const std::array<u64, 20> v11{159, 80, 12626, 2304, 4627, 58, 470, 47, 155, 536, 0, 0, 0, 3611, 58, 1472, 2139, 0, 9, 12};
  CHECK(values(cat.value().ledger()) == v11);
  CHECK_EQ(cat.value().balls(), 47u);
  CHECK_EQ(cat.value().levels().size(), 7u);
}

// Profondeurs 60 et 63 (CST-0205, coquilles de MES-M5) : coquille de 24 sites a K2/16, de 48 sites a K5/24 ; la
// seconde a huit feuilles de 48 sites (warp virtuel de 256 voies, compteurs de la voie DFS de la v11). Grands livres
// complets egaux a ceux de la v11.
MHGP12_TEST(depth_witnesses, 8) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto c24 = make_cloud(shell({1, 2, 2}, 1u << 18, 1u << 19), budget);
  REQUIRE(c24.ok());
  auto k2 = run(c24.value(), 2, 16, 2, budget);
  REQUIRE(k2.ok());
  const std::array<u64, 20> v11_24{927, 44, 108361, 3504, 23112, 443, 2684, 99, 312, 0, 0, 0, 0, 18992, 336, 18992, 0, 0, 24, 60};
  CHECK(values(k2.value().ledger()) == v11_24);
  CHECK_EQ(k2.value().balls(), 99u);
  auto c48 = make_cloud(shell({1, 2, 3}, 1u << 18, 3u << 18), budget);
  REQUIRE(c48.ok());
  CatalogueDiagnostics diag;
  auto k5 = run(c48.value(), 5, 24, 4, budget, &diag);
  REQUIRE(k5.ok());
  const std::array<u64, 20> v11_48{951, 328, 566814, 75770, 2455936, 21187, 923914, 639, 3168, 1589372, 0,
                                   4955680, 0, 5715481, 240987, 4985664, 729817, 4808288, 48, 63};
  CHECK(values(k5.value().ledger()) == v11_48);
  CHECK_EQ(k5.value().balls(), 639u);
  CHECK_EQ(diag.leaves_virtual_warp, 8u);
  CHECK_EQ(diag.leaves_narrow, 0u);  // etendue 21 : toutes les feuilles par le repli exact
  CHECK_EQ(diag.leaves_exact, diag.leaves_medium + diag.leaves_wide);
}

// Refus : parametres, multiplicites (decision D8), feuille plus large que max_leaf, budget ; aucun ne publie ni ne
// laisse de reservation.
MHGP12_TEST(refusals, 22) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  CatalogueParams p;
  p.kmax = 0;
  CHECK(check_catalogue_params(p).reason == Reason::kmax_out_of_range);
  p.kmax = 13;
  CHECK(check_catalogue_params(p).reason == Reason::kmax_out_of_range);
  p.kmax = 5;
  p.leaf_size = 7;
  CHECK(check_catalogue_params(p).reason == Reason::parameter_out_of_range);  // feuille < K + 3
  p.leaf_size = 24;
  p.max_leaf = kCatalogueMaxLeaf + 1;
  CHECK(check_catalogue_params(p).reason == Reason::parameter_out_of_range);
  p.max_leaf = 16;
  CHECK(check_catalogue_params(p).reason == Reason::parameter_out_of_range);  // feuille > max_leaf
  p.max_leaf = kCatalogueMaxLeaf;
  CHECK(check_catalogue_params(p).ok());
  // Doublon de position : un site de poids 2.
  std::vector<u32> x{1, 1, 5}, y{2, 2, 6}, z{3, 3, 7};
  std::vector<PointId> ids{make_id<PointId>(4), make_id<PointId>(9), make_id<PointId>(2)};
  auto doubled = prepare_cloud(x, y, z, ids, CoordWidth(), budget);
  REQUIRE(doubled.ok());
  auto refused = run(doubled.value(), 2, 5, 1, budget);
  CHECK(!refused.ok() && refused.outcome().reason == Reason::multiplicity_unsupported);
  CHECK(refused.outcome().status() == Status::unsupported_degeneracy);
  // Feuille plus large que max_leaf : coquille de 48 sites, feuille 8, max_leaf 8 (MES-M5, coquille48_k5_l8_m8).
  auto c48 = make_cloud(shell({1, 2, 3}, 1u << 18, 3u << 18), budget);
  REQUIRE(c48.ok());
  const u64 before = budget.used();
  auto wide = run(c48.value(), 5, 8, 2, budget, nullptr, 8);
  CHECK(!wide.ok() && wide.outcome().reason == Reason::wide_leaf);
  CHECK_EQ(budget.used(), before);
  // Budget : la moitie du pic d'une construction reussie ne suffit pas ; refus memory_budget, rien de reserve.
  MemoryBudget measure(MemoryBudget::kUnlimited);
  auto again = make_cloud(shell({1, 2, 3}, 1u << 18, 3u << 18), measure);
  REQUIRE(again.ok());
  const u64 base = measure.used();
  measure.restart_peak();
  { auto ok = run(again.value(), 5, 24, 2, measure); CHECK(ok.ok()); }
  MemoryBudget tight(base + (measure.peak() - base) / 2);
  auto small = make_cloud(shell({1, 2, 3}, 1u << 18, 3u << 18), tight);
  REQUIRE(small.ok());
  auto starved = run(small.value(), 5, 24, 2, tight);
  CHECK(!starved.ok() && starved.outcome().reason == Reason::memory_budget);
  CHECK_EQ(tight.used(), base);
  // Nom de trame de l'export : au plus 23 octets ASCII imprimables (CST-0227), sinon parameter_out_of_range.
  auto tetra = make_cloud({{0, 0, 0}, {4, 0, 0}, {0, 4, 0}, {0, 0, 4}}, budget);
  REQUIRE(tetra.ok());
  auto cat = run(tetra.value(), 2, 5, 1, budget);
  REQUIRE(cat.ok());
  CHECK(catalogue_digest(tetra.value(), cat.value(), "ng00 ~").ok());
  for (const std::string_view bad : {std::string_view("ng\x01", 3), std::string_view("ng\x7f", 3),
                                     std::string_view("ng\xff", 3), std::string_view("ng\0", 3),
                                     std::string_view("abcdefghijklmnopqrstuvwx", 24)})
    CHECK(catalogue_digest(tetra.value(), cat.value(), bad).outcome().reason == Reason::parameter_out_of_range);
}

// Somme controlee des compteurs : un depassement de 2^64 - 1 est detecte (refus catalogue_counter_overflow).
MHGP12_TEST(counter_overflow, 3) {
  catalogue_detail::LeafCounts into{}, add{};
  into.c[catalogue_detail::kPrefixes] = ~u64{0} - 1;
  add.c[catalogue_detail::kPrefixes] = 1;
  CHECK(catalogue_detail::add_leaf_counts(into, add));
  CHECK_EQ(into.c[catalogue_detail::kPrefixes], ~u64{0});
  CHECK(!catalogue_detail::add_leaf_counts(into, add));
}

namespace {

// Nuage pseudo-aleatoire de n positions distinctes de [0, top]^3 (SplitMix64), coins (0,0,0) et (top,top,top) inclus.
Points scatter(u64 n, u32 top, u64 seed) {
  Points out{{0, 0, 0}, {top, top, top}};
  u64 state = seed;
  auto next = [&]() {
    state += 0x9E3779B97F4A7C15ull;
    u64 z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
  };
  while (out.size() < n) {
    const std::array<u32, 3> p{static_cast<u32>(next() % (u64{top} + 1)), static_cast<u32>(next() % (u64{top} + 1)),
                               static_cast<u32>(next() % (u64{top} + 1))};
    if (std::find(out.begin(), out.end(), p) == out.end()) out.push_back(p);
  }
  return out;
}

std::vector<std::array<u64, 3>> positions(const Cloud& cloud, std::span<const SiteIdx> sites, u64 factor,
                                          u64 offset = 0) {
  std::vector<std::array<u64, 3>> out;
  for (SiteIdx s : sites) out.push_back({u64{cloud.x()[idx(s)]} * factor + offset, u64{cloud.y()[idx(s)]} * factor + offset,
                                         u64{cloud.z()[idx(s)]} * factor + offset});
  std::sort(out.begin(), out.end());
  return out;
}

// Meme catalogue a l'homothetie et a la translation pres : meme ordre des boules (rangs, puis S* par positions :
// invariants par homothetie positive et translation), memes p, m, q_min, memes S*, I et U en positions (celles de a,
// multipliees par factor puis translatees de offset).
bool same_up_to_scale(const Cloud& ca, const Catalogue& a, const Cloud& cb, const Catalogue& b, u64 factor,
                      u64 offset = 0) {
  if (a.balls() != b.balls() || a.levels().size() != b.levels().size()) return false;
  for (u32 i = 0; i < a.balls(); ++i) {
    const auto& x = a.balls_data()[i];
    const auto& y = b.balls_data()[i];
    if (x.rank != y.rank || x.p != y.p || x.m != y.m || x.qmin != y.qmin) return false;
    const std::span<const SiteIdx> sx(x.support.data(), x.qmin), sy(y.support.data(), y.qmin);
    if (positions(ca, sx, factor, offset) != positions(cb, sy, 1)) return false;
    const auto ball = make_id<BallIdx>(i);
    if (positions(ca, a.interior(ball), factor, offset) != positions(cb, b.interior(ball), 1)) return false;
    if (positions(ca, a.shell(ball), factor, offset) != positions(cb, b.shell(ball), 1)) return false;
  }
  return true;
}

}  // namespace

// Paliers de la feuille (CONTRAT_NUMERIQUE.md, paragraphe 7) : un nuage de 40 sites d'etendue 7 ou 8 bits (feuilles
// etroites, arithmetique native), puis le meme nuage dilate pour occuper tout le domaine du profil, [0, 2^B - 1] sur
// chaque axe (factor * top = 2^B - 1) : feuilles d'etendue au moins 17 (palier moyen, repli exact) et, au profil 32,
// au moins 25 (palier large) ; racine fermee a 2^B, soit 2^32 et s = 33 au profil 32 (CST-0204). Le catalogue doit
// etre le meme a l'homothetie pres, et les compteurs de voies doivent montrer le repli effectivement emprunte.
MHGP12_TEST(tiers, 9) {
  constexpr u32 top = kCoordBits == 21 ? 127u : 255u;
  constexpr u64 factor = ((u64{1} << kCoordBits) - 1) / top;
  static_assert(factor * top == (u64{1} << kCoordBits) - 1, "le nuage dilate touche les deux bords du domaine");
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const Points base = scatter(40, top, 20261007);
  Points dilated;
  for (const auto& p : base)
    dilated.push_back({static_cast<u32>(p[0] * factor), static_cast<u32>(p[1] * factor), static_cast<u32>(p[2] * factor)});
  auto small = make_cloud(base, budget), large = make_cloud(dilated, budget);
  REQUIRE(small.ok() && large.ok());
  CatalogueDiagnostics ds, dl;
  auto a = run(small.value(), 4, 7, 2, budget, &ds);
  auto b = run(large.value(), 4, 7, 2, budget, &dl);
  REQUIRE(a.ok() && b.ok());
  CHECK(a.value().balls() > 50u);
  CHECK(same_up_to_scale(small.value(), a.value(), large.value(), b.value(), factor));
  CHECK_EQ(ds.leaves_exact, 0u);
  CHECK(ds.max_leaf_span <= 9u);
  CHECK(dl.leaves_exact > 0u && dl.leaves_narrow == 0u);
  CHECK(dl.max_leaf_span >= 17u);
  if constexpr (kCoordBits == 32) {
    CHECK(dl.leaves_wide > 0u);
    CHECK(dl.max_leaf_span >= 25u);
  }
  CHECK(values(a.value().ledger())[7] == values(b.value().ledger())[7]);  // emises
}

// Parcours au profil 32 (fixture coquille48_u32_k5_l24 de MES-M5) : la coquille de 48 sites a l'echelle 2^29 autour
// de (2^31, 2^31, 2^31). Le repere du parent depasse 29 bits (voie large i128 du reservoir et de G1) quand la boite de
// l'enfant reste etroite : le filtre doit lire le repere du PARENT (CST-0112). Grand livre du parcours egal a celui de
// l'oracle independant de MES-M5 (oracle_parcours.py, entiers Python : 1 479 noeuds, 504 feuilles, 880 393 tests G1,
// profondeur 96 = 3B, CST-0205) ; catalogue egal a celui de la coquille de 48 sites du profil 21, a l'homothetie 2^11
// et a la translation 2^29 pres. Aux profils 21 et 24, ces coordonnees sortent du domaine : refus du nuage.
MHGP12_TEST(wide_traversal, 2) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const Points far = shell({1, 2, 3}, 1u << 29, 1u << 31);
  auto big = make_cloud(far, budget);
  if constexpr (kCoordBits < 32) {
    CHECK(!big.ok() && big.outcome().reason == Reason::coordinate_out_of_domain);
    CHECK(far.size() == 48u);
  } else {
    REQUIRE(big.ok());
    auto small = make_cloud(shell({1, 2, 3}, 1u << 18, 3u << 18), budget);
    REQUIRE(small.ok());
    auto a = run(small.value(), 5, 24, 2, budget);
    auto b = run(big.value(), 5, 24, 4, budget);
    REQUIRE(a.ok() && b.ok());
    const auto& l = b.value().ledger();
    CHECK_EQ(l.nodes, 1479u);
    CHECK_EQ(l.leaves, 504u);
    CHECK_EQ(l.filter_tests, 880393u);
    CHECK_EQ(l.max_depth, 96u);
    CHECK_EQ(l.max_leaf, 48u);
    CHECK(same_up_to_scale(small.value(), a.value(), big.value(), b.value(), u64{1} << 11, u64{1} << 29));
  }
}

// Determinisme : memes octets (empreinte canonique de l'export) a 1, 4 et 8 fils, sur un nuage pseudo-aleatoire de
// 2 500 sites et sur la coquille de 48 sites (feuilles larges et repli exact).
MHGP12_TEST(determinism, 8) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto cloud = make_cloud(scatter(2500, 65535, 7), budget);
  auto c48 = make_cloud(shell({1, 2, 3}, 1u << 18, 3u << 18), budget);
  REQUIRE(cloud.ok() && c48.ok());
  std::vector<io::Digest> digests, shell_digests;
  std::vector<CatalogueLedger> ledgers;
  for (u32 threads : {1u, 4u, 8u}) {
    auto cat = run(cloud.value(), 5, 24, threads, budget);
    REQUIRE(cat.ok());
    auto digest = catalogue_digest(cloud.value(), cat.value(), "determinisme");
    REQUIRE(digest.ok());
    digests.push_back(digest.value());
    ledgers.push_back(cat.value().ledger());
    auto wide = run(c48.value(), 5, 24, threads, budget);
    REQUIRE(wide.ok());
    auto d48 = catalogue_digest(c48.value(), wide.value(), "determinisme");
    REQUIRE(d48.ok());
    shell_digests.push_back(d48.value());
  }
  CHECK(digests[0] == digests[1] && digests[0] == digests[2]);
  CHECK(shell_digests[0] == shell_digests[1] && shell_digests[0] == shell_digests[2]);
  CHECK(ledgers[0] == ledgers[1] && ledgers[0] == ledgers[2]);
}

MHGP12_TEST_MAIN()
