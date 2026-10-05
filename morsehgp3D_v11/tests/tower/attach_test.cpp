// Rattachement de W_K (tranche S3) : fixtures gravees 1, 4, 7 et 8 du paragraphe 2.9 de la specification et temoins
// D2 et E5 du contrat (MATHEMATIQUES 10.3 et 10.11), attendus recalcules en Fraction par l'oracle borne S1
// (reference/hgp11_ref/supports.py, definition : composantes de Gamma_K a toutes les K-parties) ; puis E1 = E2 sur des
// nuages bornes : descente d'une K-partie et ancetre ferme (lemme E), seconde K-partie (T3, I7), branches recomptees
// par descentes neuves des traces strictes, I1 a I4 recomptes hors du produit ; garde de capacite des traces publiees.
#include "order_tree_support.hpp"
#include "test.hpp"
#include "tower/seed_log.hpp"
using namespace order_tree_test;

namespace {
struct Expected {
  std::vector<Xyz> support;  // sites de S*, ordre libre
  i64 num, den;              // niveau exact
  BallRole role;
  u32 node, strict, components;
  std::vector<u32> prior;    // role fusion seulement
};
using Graved = std::pair<std::vector<Xyz>, std::array<i64, 2>>;  // partie (ou S*), niveau exact num/den
struct Fixture {
  const char* name; std::vector<Xyz> points; Order k; std::vector<Expected> balls;
  std::vector<std::vector<Xyz>> outside = {};  // supports sans boule du catalogue : boule minimale hors de Cat_K
  std::vector<Graved> meb_levels = {};         // niveau de la boule minimale d'une partie (MEB bornee)
  std::vector<Graved> open_cuts = {};          // niveau l(r_b - 1) de la coupe ouverte d'une boule de S* donne
  std::vector<std::pair<u32, std::vector<u32>>> children = {};  // enfants graves de fusions
};

constexpr BallRole B = BallRole::birth, M = BallRole::merge, I = BallRole::internal;
std::vector<Fixture> fixtures() {
  const std::vector<Xyz> square{{0,0,0},{2,0,0},{2,2,0},{0,2,0}};
  const std::vector<Xyz> passing{{0,0,0},{2,2,0},{4,0,0},{8,0,0}};
  const std::vector<Xyz> tetra{{20,20,20},{20,0,0},{0,20,0},{0,0,20},{10,10,10},{11,10,10},{10,11,10}};
  const Xyz a{2,10,0}, b{18,10,0}, c{10,20,0}, z{9,3,0}, w{11,3,0};             // temoin D2
  const Xyz ea{0,0,7}, eb{0,9,6}, ec{1,4,0}, ed{0,0,1}, ee{4,1,2};               // temoin E5
  return {
    {"carre_k1", square, 1, {{{{0,0,0},{2,0,0}}, 1, 1, M, 4, 2, 2, {0, 2}}, {{{0,0,0},{0,2,0}}, 1, 1, M, 4, 2, 2, {0, 1}},
                            {{{2,0,0},{2,2,0}}, 1, 1, M, 4, 2, 2, {2, 3}}, {{{0,2,0},{2,2,0}}, 1, 1, M, 4, 2, 2, {1, 3}},
                            {{{0,0,0},{2,2,0}}, 2, 1, I, 4, 4, 1, {}}}},
    {"carre_k2", square, 2, {{{{0,0,0},{2,0,0}}, 1, 1, B, 1, 0, 0, {}}, {{{0,0,0},{0,2,0}}, 1, 1, B, 0, 0, 0, {}},
                            {{{2,0,0},{2,2,0}}, 1, 1, B, 3, 0, 0, {}}, {{{0,2,0},{2,2,0}}, 1, 1, B, 2, 0, 0, {}},
                            {{{0,0,0},{2,2,0}}, 2, 1, M, 4, 4, 4, {0, 1, 2, 3}}}},
    {"carre_k3", square, 3, {{{{0,0,0},{2,2,0}}, 2, 1, B, 0, 0, 0, {}}}},
    {"carre_k4", square, 4, {{{{0,0,0},{2,2,0}}, 2, 1, B, 0, 0, 0, {}}}},
    // Cellule passagere : {a,b,c} au niveau 4 (diametre ac, b sur le cercle) est de role fusion a une seule branche.
    {"passagere_k1", passing, 1, {{{{0,0,0},{2,2,0}}, 2, 1, M, 4, 2, 2, {0, 1}},
                                 {{{2,2,0},{4,0,0}}, 2, 1, M, 4, 2, 2, {1, 2}},
                                 {{{0,0,0},{4,0,0}}, 4, 1, M, 5, 3, 1, {4}},
                                 {{{4,0,0},{8,0,0}}, 4, 1, M, 5, 2, 2, {3, 4}}}},
    // Evenement faible (p+q-1 = K) : trois naissances de paires au niveau 2, fusion a trois enfants au niveau 8/3.
    {"equilateral_k2", {{0,0,0},{2,2,0},{2,0,2}}, 2,
     {{{{0,0,0},{2,2,0}}, 2, 1, B, 1, 0, 0, {}}, {{{0,0,0},{2,0,2}}, 2, 1, B, 0, 0, 0, {}},
      {{{2,2,0},{2,0,2}}, 2, 1, B, 2, 0, 0, {}}, {{{0,0,0},{2,2,0},{2,0,2}}, 8, 3, M, 3, 3, 3, {0, 1, 2}}}},
    // Plateau K5 : six composantes au niveau 200, quatre boules de face au niveau 800/3 (trois branches chacune),
    // une fusion a six enfants ; aucune donnee d'unions effectuees.
    {"tetraedre_k5", tetra, 5,
     {{{{20,0,0},{0,20,0}}, 200, 1, B, 2, 0, 0, {}}, {{{20,0,0},{0,0,20}}, 200, 1, B, 1, 0, 0, {}},
      {{{20,0,0},{20,20,20}}, 200, 1, B, 5, 0, 0, {}}, {{{0,20,0},{0,0,20}}, 200, 1, B, 0, 0, 0, {}},
      {{{0,20,0},{20,20,20}}, 200, 1, B, 4, 0, 0, {}}, {{{0,0,20},{20,20,20}}, 200, 1, B, 3, 0, 0, {}},
      {{{20,0,0},{0,20,0},{0,0,20}}, 800, 3, M, 6, 3, 3, {0, 1, 2}},
      {{{20,0,0},{0,20,0},{20,20,20}}, 800, 3, M, 6, 3, 3, {2, 4, 5}},
      {{{20,0,0},{0,0,20},{20,20,20}}, 800, 3, M, 6, 3, 3, {1, 3, 5}},
      {{{0,20,0},{0,0,20},{20,20,20}}, 800, 3, M, 6, 3, 3, {0, 3, 4}}}},
    // Temoin D2 (auditeur, de4ab58a8) : la boule faible ABC (niveau 1681/25, p = 0, q = m = 3) fusionne trois
    // branches ; sa trace stricte AB a une boule minimale de niveau 64 hors de Cat_2 (Z et W interieurs), nee apres le
    // niveau 41 de rang r_b - 1 : aucune garde beta(F) <= l(r_b - 1) n'est admise (MATHEMATIQUES 10.2 et 10.5).
    {"temoin_d2_k2", witness_d2(), 2,
     {{{z, w}, 1, 1, B, 0, 0, 0, {}}, {{a, z}, 49, 2, B, 1, 0, 0, {}}, {{w, b}, 49, 2, B, 2, 0, 0, {}},
      {{a, w}, 65, 2, M, 5, 2, 2, {0, 1}}, {{z, b}, 65, 2, M, 5, 2, 2, {0, 2}},
      {{a, c}, 41, 1, B, 3, 0, 0, {}}, {{c, b}, 41, 1, B, 4, 0, 0, {}},
      {{a, b, c}, 1681, 25, M, 6, 3, 3, {3, 4, 5}},
      {{z, c}, 145, 2, I, 6, 2, 1, {}}, {{w, c}, 145, 2, I, 6, 2, 1, {}}},
     {{a, b}}, {{{a, b}, {64, 1}}}, {{{a, b, c}, {41, 1}}}, {{5, {0, 1, 2}}, {6, {3, 4, 5}}}},
    // Temoin E5 du lemme W, point 2 : la boule de AC (niveau 33/2, interieurs D et E) est hors fenetre ; ses liaisons
    // rattachent AC a la composante de DE, si bien que la fusion de niveau 83886/3563 a trois enfants : {AB} (noeud 6),
    // {AC, AD, AE, CD, CE, DE} (fusion 8) et {BC} (noeud 5). Le calcul lit le vrai Gamma_K, jamais W_K seul.
    {"temoin_e5_k2", witness_e5(), 2,
     {{{ed, ee}, 9, 2, B, 1, 0, 0, {}}, {{ed, ec}, 9, 2, B, 0, 0, 0, {}}, {{ee, ec}, 11, 2, B, 2, 0, 0, {}},
      {{ed, ee, ec}, 162, 25, M, 7, 3, 3, {0, 1, 2}},
      {{ed, ea}, 9, 1, B, 3, 0, 0, {}}, {{ee, ea}, 21, 2, B, 4, 0, 0, {}},
      {{ed, ee, ea}, 189, 17, M, 8, 3, 3, {3, 4, 7}},
      {{ec, eb}, 31, 2, B, 5, 0, 0, {}}, {{ea, eb}, 41, 2, B, 6, 0, 0, {}},
      {{ec, ea, eb}, 83886, 3563, M, 9, 3, 3, {5, 6, 8}},
      {{ee, eb}, 24, 1, I, 9, 2, 1, {}}, {{ed, eb}, 53, 2, I, 9, 2, 1, {}}},
     {{ea, ec}}, {{{ea, ec}, {33, 2}}}, {}, {{7, {0, 1, 2}}, {8, {3, 4, 7}}, {9, {5, 6, 8}}}},
  };
}

std::vector<u32> sites_of(const FullDomain& domain, const std::vector<Xyz>& points) {
  std::vector<u32> out;
  for (const auto& p : points) out.push_back(idx(site(domain.index().cloud(), p)));
  std::sort(out.begin(), out.end());
  return out;
}
std::vector<u32> support_of(const CatalogueBall& data) {
  std::vector<u32> own;
  for (u32 j = 0; j < data.qmin; ++j) own.push_back(idx(data.support[j]));
  return own;
}
// Position dans W_K de la boule de support donne (ensemble de sites), ou kNone.
u64 position_of(const OrderTree& tree, const std::vector<Xyz>& support) {
  const auto& domain = tree.domain();
  const auto wanted = sites_of(domain, support);
  const auto window = tree.attachment().balls();
  for (u64 at = 0; at < window.size(); ++at)
    if (support_of(domain.catalogue().balls_data()[idx(window[at])]) == wanted) return at;
  return kNone;
}
// Une boule du catalogue (Cat_K entier, pas seulement W_K) a-t-elle ce support canonique ?
bool in_catalogue(const OrderTree& tree, const std::vector<Xyz>& support) {
  const auto wanted = sites_of(tree.domain(), support);
  for (const auto& data : tree.domain().catalogue().balls_data()) if (support_of(data) == wanted) return true;
  return false;
}
bool level_equal(const num::Level& level, const std::array<i64, 2>& expected) {
  return level_is(level, expected[0], expected[1]);
}
// Controles des temoins : boules hors de Cat_K, niveaux de boules minimales, coupes ouvertes, enfants graves.
void witness_checks(const Fixture& fixture, const OrderTree& tree, u64& checked) {
  const auto& domain = tree.domain();
  const auto levels = domain.catalogue().levels();
  for (const auto& support : fixture.outside) { CHECK(!in_catalogue(tree, support)); ++checked; }
  for (const auto& [part, level] : fixture.meb_levels) {
    std::vector<SiteIdx> ids;
    for (u32 s : sites_of(domain, part)) ids.push_back(SiteIdx{s});
    auto meb = bounded_meb(domain.index().cloud(), ids); REQUIRE(meb.ok());
    CHECK(level_equal(meb.value().sphere().level(), level)); ++checked;
  }
  for (const auto& [support, level] : fixture.open_cuts) {
    const u64 at = position_of(tree, support); REQUIRE(at != kNone);
    const auto rank = domain.catalogue().balls_data()[idx(tree.attachment().balls()[at])].rank;
    REQUIRE(idx(rank) >= 1);
    CHECK(level_equal(levels[idx(rank) - 1], level)); ++checked;
  }
  for (const auto& [node, kids] : fixture.children) {
    REQUIRE(node < tree.forest().nodes().size());
    std::vector<u32> got;
    for (NodeIdx child : tree.forest().children(NodeIdx{node})) got.push_back(idx(child));
    CHECK(got == kids); ++checked;
  }
}
}  // namespace

MHGP11_TEST(fixtures, 823) {
  auto pool = sched::make_pool({4}); REQUIRE(pool.ok());
  FullParams batch; batch.regular_batch_capacity = 4096; batch.descent_lanes = 4; batch.population_lookup = true;
  batch.dense_birth_lookup = true; batch.reuse_census_workspace = true;
  u64 checked = 0, witnesses = 0;
  for (const auto& fixture : fixtures()) {
    for (bool lots : {false, true}) {
      MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
      auto domain = domain_of(Input(fixture.points), owner, fixture.k); REQUIRE(domain.ok());
      auto tree = build_order(std::move(domain.value()), fixture.k, work, lots ? batch : FullParams{},
                              lots ? pool.value().get() : nullptr);
      if (!tree.ok()) std::printf("refus %s raison=%s\n", fixture.name,
                                  std::string(reason_name(tree.outcome().reason)).c_str());
      REQUIRE(tree.ok());
      const auto& a = tree.value().attachment();
      const auto levels = tree.value().domain().catalogue().levels();
      CHECK_EQ(a.size(), fixture.balls.size());
      for (const auto& e : fixture.balls) {
        const u64 at = position_of(tree.value(), e.support);
        REQUIRE(at != kNone);
        const auto& data = tree.value().domain().catalogue().balls_data()[idx(a.balls()[at])];
        CHECK(level_is(levels[idx(data.rank)], e.num, e.den));
        CHECK(a.role()[at] == e.role);
        CHECK_EQ(idx(a.node()[at]), e.node);
        CHECK_EQ(a.strict_traces()[at], e.strict);
        CHECK_EQ(a.components()[at], e.components);
        const auto begin = a.prior_offsets()[at], end = a.prior_offsets()[at + 1];
        std::vector<u32> prior;
        for (u64 j = begin; j < end; ++j) prior.push_back(idx(a.prior()[j]));
        CHECK(prior == e.prior);
        ++checked;
      }
      witness_checks(fixture, tree.value(), witnesses);
    }
  }
  CHECK_EQ(checked, 2u * 52u); CHECK_EQ(witnesses, 2u * 10u);
}

// Garde de capacite des traces publiees en u32 (audit aef7182b3), jouee directement sur des valeurs synthetiques : le
// cas reel, une coquille entiere de 150 sites a K8 (au moins C(69,8) = 8 361 453 672 traces strictes), est hors de
// portee d'un test. Refus tower_capacity au-dela de UINT32_MAX, aucune valeur tronquee.
MHGP11_TEST(capacity, 12) {
  auto top = published_traces(u64{0xFFFFFFFFu}); REQUIRE(top.ok()); CHECK_EQ(top.value(), 0xFFFFFFFFu);
  auto small = published_traces(3); REQUIRE(small.ok()); CHECK_EQ(small.value(), 3u);
  auto over = published_traces(u64{1} << 32); CHECK(!over.ok()); CHECK_EQ(over.outcome().reason, Reason::tower_capacity);
  auto huge = cell_binomial(69, 8); REQUIRE(huge.ok()); CHECK_EQ(huge.value(), u64{8361453672});
  auto audit = published_traces(huge.value());
  CHECK(!audit.ok()); CHECK_EQ(audit.outcome().reason, Reason::tower_capacity);
  auto last = published_traces(~u64{0}); CHECK(!last.ok()); CHECK_EQ(last.outcome().reason, Reason::tower_capacity);
}

// Petit temoin a K eleve de l'auditeur (integration L1 ; receipts/audit_native_integration_20261005/qb/normal.json) :
// les quatre coins d'un carre de cote 20 et huit sites interieurs, domaine prepare a l'ordre K (kmax = K, domaine
// etroit de la facade), voies serielle et lots W4. La boule de centre (10, 10, 10) et de niveau 200 (p = 8, m = 4,
// qmin = 2 ; quatre traces strictes a K10 selon le recu) est interne a K9, fusion des quatre naissances de niveau 101
// a K10 (dont deux sur des coquilles etendues, m = 4 > qmin = 3), naissance a K11 et K12. Attendus de l'oracle borne
// S1 (Supports.canonical, Fraction), jamais du produit ; chaque boule est reperee par sa coquille U_b, egale ici a
// l'union de ses supports (m = taille de cette union pour chacune). La suite des petits nuages s'arrete a K5 : avec la
// porte mhgp11_tower_attach_fraction (K1 a K12), c'est le seul cas borne de l'arbre et du rattachement a K >= 6.
namespace {
struct ShellBall {
  std::vector<Xyz> shell;    // U_b, ordre libre
  i64 num, den;              // niveau exact
  u32 p, m, q;
  BallRole role;
  u32 node, strict, components;
  std::vector<u32> prior;    // role fusion seulement
};
struct HighOrder {
  Order k; u32 nodes, births;
  std::pair<u32, std::vector<u32>> merge;  // fusion gravee et ses enfants (noeud kNone : aucune)
  std::vector<ShellBall> balls;
};
std::vector<Xyz> witness_k10() {
  return {{0,0,10},{0,20,10},{20,0,10},{20,20,10},{9,9,10},{9,10,10},{9,11,10},{10,9,10},{10,10,10},{10,11,10},
          {11,9,10},{11,10,10}};
}
std::vector<HighOrder> high_orders() {
  const Xyz o{0,0,10}, n{0,20,10}, e{20,0,10}, ne{20,20,10};                       // coins
  const Xyz a{9,9,10}, b{9,11,10}, c{10,11,10}, d{11,9,10}, f{11,10,10};          // sites interieurs utiles
  const std::vector<Xyz> corners{o, n, e, ne};
  return {
    {9, 7, 6, {6, {0, 1, 2, 3, 4, 5}},
     {{{o, c, f}, 48841, 882, 6, 3, 3, B, 0, 0, 0, {}}, {{n, d}, 121, 2, 7, 2, 2, B, 1, 0, 0, {}},
      {{b, e}, 121, 2, 7, 2, 2, B, 2, 0, 0, {}}, {{a, ne}, 121, 2, 7, 2, 2, B, 3, 0, 0, {}},
      {{o, n, f}, 48841, 484, 6, 3, 3, B, 4, 0, 0, {}}, {{o, c, e}, 48841, 484, 6, 3, 3, B, 5, 0, 0, {}},
      {{o, n, d}, 101, 1, 7, 3, 3, M, 6, 3, 3, {0, 1, 4}}, {{o, b, e}, 101, 1, 7, 3, 3, M, 6, 3, 3, {0, 2, 5}},
      {{n, a, ne, d}, 101, 1, 6, 4, 3, M, 6, 2, 2, {1, 3}}, {{a, e, ne, b}, 101, 1, 6, 4, 3, M, 6, 2, 2, {2, 3}},
      {corners, 200, 1, 8, 4, 2, I, 6, 4, 1, {}}}},
    {10, 5, 4, {4, {0, 1, 2, 3}},
     {{{o, n, d}, 101, 1, 7, 3, 3, B, 0, 0, 0, {}}, {{o, b, e}, 101, 1, 7, 3, 3, B, 1, 0, 0, {}},
      {{n, a, ne, d}, 101, 1, 6, 4, 3, B, 2, 0, 0, {}}, {{a, e, ne, b}, 101, 1, 6, 4, 3, B, 3, 0, 0, {}},
      {corners, 200, 1, 8, 4, 2, M, 4, 4, 4, {0, 1, 2, 3}}}},
    {11, 1, 1, {kNone, {}}, {{corners, 200, 1, 8, 4, 2, B, 0, 0, 0, {}}}},
    {12, 1, 1, {kNone, {}}, {{corners, 200, 1, 8, 4, 2, B, 0, 0, 0, {}}}},
  };
}
}  // namespace

MHGP11_TEST(square_k10, 446) {
  auto pool = sched::make_pool({4}); REQUIRE(pool.ok());
  FullParams batch; batch.regular_batch_capacity = 4096; batch.descent_lanes = 4; batch.population_lookup = true;
  batch.dense_birth_lookup = true; batch.reuse_census_workspace = true;
  u64 judged = 0;
  for (const auto& order : high_orders()) {
    for (bool lots : {false, true}) {
      MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited);
      auto domain = domain_of(Input(witness_k10()), owner, static_cast<int>(order.k)); REQUIRE(domain.ok());
      auto tree = build_order(std::move(domain.value()), order.k, work, lots ? batch : FullParams{},
                              lots ? pool.value().get() : nullptr);
      REQUIRE(tree.ok());
      const auto& forest = tree.value().forest();
      const auto& a = tree.value().attachment();
      const auto& cat = tree.value().domain().catalogue();
      CHECK_EQ(forest.nodes().size(), std::size_t{order.nodes}); CHECK_EQ(forest.births(), order.births);
      CHECK_EQ(a.size(), order.balls.size());
      if (order.merge.first != kNone) {
        REQUIRE(order.merge.first < forest.nodes().size());
        std::vector<u32> kids;
        for (NodeIdx child : forest.children(NodeIdx{order.merge.first})) kids.push_back(idx(child));
        CHECK(kids == order.merge.second);
      }
      for (const auto& e : order.balls) {
        const auto wanted = sites_of(tree.value().domain(), e.shell);
        u64 at = kNone;
        for (u64 j = 0; j < a.size(); ++j) {
          std::vector<u32> shell;
          for (SiteIdx s : cat.shell(a.balls()[j])) shell.push_back(idx(s));
          std::sort(shell.begin(), shell.end());
          if (shell == wanted) { CHECK(at == kNone); at = j; }
        }
        REQUIRE(at != kNone);
        const auto& data = cat.balls_data()[idx(a.balls()[at])];
        CHECK(level_is(cat.levels()[idx(data.rank)], e.num, e.den));
        CHECK_EQ(u32{data.p}, e.p); CHECK_EQ(u32{data.m}, e.m); CHECK_EQ(u32{data.qmin}, e.q);
        CHECK(a.role()[at] == e.role);
        CHECK_EQ(idx(a.node()[at]), e.node);
        CHECK_EQ(a.strict_traces()[at], e.strict);
        CHECK_EQ(a.components()[at], e.components);
        std::vector<u32> prior;
        for (u64 j = a.prior_offsets()[at]; j < a.prior_offsets()[at + 1]; ++j) prior.push_back(idx(a.prior()[j]));
        CHECK(prior == e.prior);
        ++judged;
      }
    }
  }
  CHECK_EQ(judged, 2u * 18u);
}

// E1 = E2 sur toutes les boules des nuages bornes, voie serielle et lots W4, sur deux domaines : Cat_kmax du nuage
// (large) et Cat_K pour K < kmax (etroit : configuration de la facade, domaine prepare a l'ordre demande). Le
// contre-cas D2, une trace stricte nee apres l(r_b - 1), n'apparait que sur le domaine etroit : a kmax >= 3 la boule
// de AB entre au catalogue, et l(r_b - 1) vaut 64 au lieu de 41 pour ABC (contre-lecture S3). I1 a I4 recomptes hors
// du produit.
namespace {
struct E1E2Counts {
  u64 judged = 0, births = 0, merges = 0, internals = 0, passing = 0, wide = 0, extended = 0, weak = 0, late = 0;
};
void print_counts(const char* domain, const E1E2Counts& c) {
  std::printf("attach_e1e2 domaine=%s boules=%llu naissances=%llu fusions=%llu internes=%llu passageres=%llu "
              "fusions_3plus=%llu etendues=%llu faibles=%llu tardives=%llu\n", domain,
              (unsigned long long)c.judged, (unsigned long long)c.births, (unsigned long long)c.merges,
              (unsigned long long)c.internals, (unsigned long long)c.passing, (unsigned long long)c.wide,
              (unsigned long long)c.extended, (unsigned long long)c.weak, (unsigned long long)c.late);
}
}  // namespace

MHGP11_TEST(e1e2, 1090000) {
  auto pool = sched::make_pool({4}); REQUIRE(pool.ok());
  FullParams batch; batch.regular_batch_capacity = 64; batch.descent_lanes = 4; batch.memo_capacity = 8;
  batch.lane_memo_capacity = 8; batch.population_lookup = true;
  std::array<E1E2Counts, 2> counted{};  // 0 : domaine large Cat_kmax ; 1 : domaine etroit Cat_K, K < kmax
  for (const auto& cloud : clouds()) for (bool lots : {false, true}) for (Order k = 1; k <= cloud.kmax; ++k)
    for (u32 pass = 0; pass < 2; ++pass) {
    if (pass == 1 && k == cloud.kmax) continue;  // meme domaine que la passe large
    auto& [judged, births, merges, internals, passing, wide, extended, weak, late] = counted[pass];
    MemoryBudget owner(MemoryBudget::kUnlimited), work(MemoryBudget::kUnlimited), judge(MemoryBudget::kUnlimited);
    auto domain = domain_of(Input(cloud.points), owner, pass == 0 ? cloud.kmax : k); REQUIRE(domain.ok());
    auto tree = build_order(std::move(domain.value()), k, work, lots ? batch : FullParams{},
                            lots ? pool.value().get() : nullptr);
    REQUIRE(tree.ok());
    const auto& d = tree.value().domain();
    const auto& forest = tree.value().forest();
    const auto& a = tree.value().attachment();
    const auto nodes = forest.nodes();
    const auto balls = d.catalogue().balls_data();
    // I1 : W_K recalcule, BallIdx croissants, naissances de la foret (K >= 2).
    u64 window = 0;
    for (const auto& ball : balls) window += window_ball(ball, k) ? 1 : 0;
    CHECK_EQ(a.size(), window);
    u64 sum_s = 0, sum_c = 0, cells = 0, touched = 0, continuing = 0, own_births = 0;
    std::set<u32> ranks;
    std::vector<std::set<u32>> covered(nodes.size());
    std::vector<std::pair<u32, u32>> touched_at, internal_at;  // (rang, noeud)
    for (u64 at = 0; at < a.size(); ++at) {
      const BallIdx ball = a.balls()[at];
      const auto& data = balls[idx(ball)];
      const NodeIdx att = a.node()[at];
      CHECK(at == 0 || idx(a.balls()[at - 1]) < idx(ball));
      CHECK(window_ball(data, k));
      // Lemme E et T3 : deux K-parties de P_b donnent le meme noeud ferme.
      auto first = judge_e2(d, forest, ball, true, judge); REQUIRE(first.ok());
      auto last = judge_e2(d, forest, ball, false, judge); REQUIRE(last.ok());
      CHECK(first.value() == att); CHECK(last.value() == att);
      if (data.m != data.qmin) ++extended;
      // Contrat D.4 : une naissance est forte (p+q <= K), une boule faible (p+q-1 = K) n'est jamais une naissance.
      if (u64{data.p} + data.qmin == u64{k} + 1) { CHECK(a.role()[at] != BallRole::birth); ++weak; }
      if (a.role()[at] == BallRole::birth) {
        CHECK(u64{data.p} + data.qmin <= k);
        CHECK(k >= 2 && idx(att) < forest.births() && nodes[idx(att)].birth_key == idx(ball));
        CHECK(nodes[idx(att)].rank == data.rank);
        CHECK_EQ(a.strict_traces()[at], 0u); CHECK_EQ(a.components()[at], 0u);
        CHECK_EQ(a.prior_offsets()[at + 1], a.prior_offsets()[at]);
        ++own_births; ++judged;
        continue;
      }
      u64 traces = 0;
      auto branches = judge_branches(d, forest, ball, traces, late, judge); REQUIRE(branches.ok());
      CHECK_EQ(a.strict_traces()[at], traces); CHECK_EQ(a.components()[at], branches.value().size());
      sum_s += traces; ++cells; ranks.insert(idx(data.rank));
      auto universe = cell_binomial(data.m, k - data.p); REQUIRE(universe.ok());
      sum_c += universe.value();
      for (u32 u : branches.value()) touched_at.push_back({idx(data.rank), u});
      if (a.role()[at] == BallRole::merge) {
        // Lemmes B et C : fusion creee au rang de la boule, branches parmi ses enfants, publiees.
        CHECK(idx(att) >= forest.births() && nodes[idx(att)].rank == data.rank);
        std::vector<u32> prior;
        for (u64 j = a.prior_offsets()[at]; j < a.prior_offsets()[at + 1]; ++j) prior.push_back(idx(a.prior()[j]));
        CHECK(prior == branches.value());
        for (u32 u : prior) { CHECK(nodes[u].parent == att); covered[idx(att)].insert(u); }
        ++merges; passing += prior.size() == 1 ? 1 : 0; wide += prior.size() >= 3 ? 1 : 0;
      } else {
        CHECK(a.role()[at] == BallRole::internal);
        CHECK(branches.value() == std::vector<u32>{idx(att)});
        CHECK(idx(nodes[idx(att)].rank) < idx(data.rank));
        CHECK(nodes[idx(att)].parent == NodeIdx{kNone} || idx(nodes[idx(nodes[idx(att)].parent)].rank) > idx(data.rank));
        CHECK_EQ(a.prior_offsets()[at + 1], a.prior_offsets()[at]);
        internal_at.push_back({idx(data.rank), idx(att)});
        ++internals;
      }
      ++judged;
    }
    // I3 : chaque fusion recoit toutes ses branches par ses boules de role fusion.
    for (u32 v = forest.births(); v < nodes.size(); ++v) {
      const auto kids = forest.children(NodeIdx{v});
      std::set<u32> expected;
      for (NodeIdx c : kids) expected.insert(idx(c));
      CHECK(covered[v] == expected);
    }
    // I4 : registres de la foret.
    std::sort(touched_at.begin(), touched_at.end());
    touched = static_cast<u64>(std::unique(touched_at.begin(), touched_at.end()) - touched_at.begin());
    std::sort(internal_at.begin(), internal_at.end());
    continuing = static_cast<u64>(std::unique(internal_at.begin(), internal_at.end()) - internal_at.begin());
    const auto& ledger = forest.ledger();
    CHECK_EQ(sum_s, ledger.trace_resolutions); CHECK_EQ(sum_c, ledger.cells.combinations);
    CHECK_EQ(cells, ledger.replayed_cells); CHECK_EQ(ranks.size(), ledger.plateaus);
    CHECK_EQ(touched, ledger.touched_components); CHECK_EQ(continuing, ledger.continuations);
    CHECK_EQ(own_births, k == 1 ? 0u : forest.births());
    births += own_births;
  }
  // Planchers graves (voies serielle et par lots comptees chacune) : passageres, fusions a trois branches ou plus,
  // coquilles etendues, internes et boules faibles presentes en nombre. Traces nees apres l(r_b - 1), une par voie
  // et par cellule : passe large, la grappe de 120 points a K = kmax = 5 (domaine qui est aussi Cat_K) ; passe
  // etroite, le temoin D2 a K = 2 (trace AB, niveau 64 apres 41), le nuage de 70 points a K = 2 et celui de 60 points
  // a K = 3 et 4. Aucune a kmax > K sur ces nuages.
  print_counts("large", counted[0]); print_counts("etroit", counted[1]);
  const auto& [judged, births, merges, internals, passing, wide, extended, weak, late] = counted[0];
  CHECK_EQ(judged, 45896u); CHECK_EQ(births, 17216u); CHECK_EQ(merges, 15886u); CHECK_EQ(internals, 12794u);
  CHECK_EQ(passing, 3196u); CHECK_EQ(wide, 4588u); CHECK_EQ(extended, 5884u); CHECK_EQ(weak, 26276u);
  CHECK_EQ(late, 2u);
  const E1E2Counts& narrow = counted[1];
  CHECK_EQ(narrow.judged, 30088u); CHECK_EQ(narrow.births, 10494u); CHECK_EQ(narrow.merges, 10624u);
  CHECK_EQ(narrow.internals, 8970u); CHECK_EQ(narrow.passing, 2306u); CHECK_EQ(narrow.wide, 2738u);
  CHECK_EQ(narrow.extended, 4330u); CHECK_EQ(narrow.weak, 17866u); CHECK_EQ(narrow.late, 8u);
}

MHGP11_TEST_MAIN()
