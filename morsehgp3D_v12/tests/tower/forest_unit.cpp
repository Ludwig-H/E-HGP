// Portes unitaires des etages T, M, V et R (docs/CONTRAT_TOUR.md, paragraphe 9) : hypergraphes aleatoires a rangs
// tres repetes et cibles << cellule >> contre un Kruskal par lots (port de la porte de MES-M4), temoins de plateau
// (WIT-SIX, WIT-TRI-EQ, plateaux disjoints, REG:238), element de cellule relu a sa racine courante, cellule inerte,
// profondeur d'attache (union par taille, LEM-T5 (i)), domaine des operandes (CST-0212), requete hors domaine de
// LEM-T5 (CST-0105, tower_query_domain), verticales du carre K1..4 (CST-0214) et remontee d'un cran de LEM-T6,
// determinisme (fils et tranches), refus transactionnels (tower_capacity, tower_invariant), admission memoire par etage
// (pic mesure de T, M, V et R contre leurs octets admis).
#include <sys/mman.h>

#include <bit>
#include <random>

#include "forest_support.hpp"
#include "tower/forest_internal.hpp"

namespace mhgp12::tower_test {
namespace {

MHGP12_TEST(hypergraphes, 6000) {
  std::mt19937_64 rng(20261007);
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = pool_of(3);
  u64 nary = 0, cell_targets = 0;
  for (int it = 0; it < 1500; ++it) {
    const u32 nb = 2 + static_cast<u32>(rng() % 39);
    const OrderData d = random_order(rng, nb, 1 + static_cast<u32>(rng() % (3 * nb)));
    const Cloud cloud = line_cloud(nb, budget);
    tower::ForestParams params;
    params.slice_events = 1 + static_cast<u32>(rng() % 5);
    Run r = run(cloud, {d}, budget, *pool, {}, params);
    REQUIRE(r.forests.ok());
    const Reference ref = kruskal_lots(d);
    CHECK(same_forest(r.forests.value().orders[0], ref));
    CHECK(tower::validate_forests(r.forests.value(), budget).ok());
    CHECK_EQ(r.ledger.work[0].events, nb - 1);
    CHECK(r.ledger.work[0].max_attach_depth <= static_cast<u64>(std::bit_width(u64{nb}) - 1));
    for (u32 v = nb; v < r.forests.value().orders[0].nodes(); ++v) nary += ref.kids[v].size() >= 3;
    cell_targets += r.ledger.work[0].cell_targets;
  }
  CHECK(nary >= 1000);
  CHECK(cell_targets >= 1000);
  std::printf("hypergraphes fusions_nary=%llu cibles_cellule=%llu\n", static_cast<unsigned long long>(nary),
              static_cast<unsigned long long>(cell_targets));
}

// Temoin : entree, fusions attendues (rang, enfants) apres les naissances.
struct Witness {
  const char* name;
  OrderData data;
  std::vector<std::pair<u32, std::vector<u32>>> merges;
};

std::vector<Witness> witnesses() {
  std::vector<Witness> out;
  auto add = [&out](const char* name, u32 nb, std::vector<std::pair<u32, std::vector<u32>>> cells,
                    std::vector<std::pair<u32, std::vector<u32>>> merges) {
    Witness w{name, {}, merges};
    w.data.births(nb);
    for (auto& [rank, targets] : cells) w.data.cell(rank, targets);
    out.push_back(w);
  };
  add("ternaire_une_cellule", 3, {{1, {0, 1, 2}}}, {{1, {0, 1, 2}}});                         // WIT-SIX
  add("plateau_chaine", 3, {{1, {0, 1}}, {1, {1, 2}}}, {{1, {0, 1, 2}}});
  add("cycle_tri_eq", 3, {{1, {0, 1}}, {1, {1, 2}}, {1, {0, 2}}}, {{1, {0, 1, 2}}});       // WIT-TRI-EQ
  add("plateaux_disjoints", 5, {{1, {0, 1}}, {1, {2, 3}}, {2, {3, 4}}, {3, {0, 4}}},
      {{1, {0, 1}}, {1, {2, 3}}, {2, {4, 6}}, {3, {5, 7}}});
  add("reg238_etoile", 5, {{1, {1, 2}}, {1, {3, 4}}, {2, {2, 3}}, {3, {0, 2, 3}}},          // REG:238
      {{1, {1, 2}}, {1, {3, 4}}, {2, {5, 6}}, {3, {0, 7}}});
  add("element_racine_courante", 5, {{1, {0, 1}}, {2, {2, 3}}, {3, {1, 2}}, {4, {4, cell_target(1)}}},
      {{1, {0, 1}}, {2, {2, 3}}, {3, {5, 6}}, {4, {4, 7}}});
  add("cellule_inerte", 3, {{1, {0, 1}}, {2, {cell_target(0)}}, {3, {cell_target(1), 2}}},
      {{1, {0, 1}}, {3, {2, 3}}});
  return out;
}

MHGP12_TEST(temoins, 50) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = pool_of(2);
  for (const Witness& w : witnesses()) {
    const u32 nb = static_cast<u32>(w.data.birth_key.size());
    const Cloud cloud = line_cloud(nb, budget);
    Run r = run(cloud, {w.data}, budget, *pool);
    if (!CHECK(r.forests.ok())) {
      std::printf("      temoin %s refuse : %s\n", w.name, std::string(reason_name(r.forests.outcome().reason)).c_str());
      continue;
    }
    const OrderForest& f = r.forests.value().orders[0];
    bool same = f.nodes() == nb + w.merges.size();
    for (u32 j = 0; same && j < w.merges.size(); ++j) {
      const u32 v = nb + j;
      const u64 b = f.children.off[v], e = f.children.off[u64{v} + 1];
      same = f.rank[v] == w.merges[j].first && e - b == w.merges[j].second.size() &&
             std::equal(w.merges[j].second.begin(), w.merges[j].second.end(), f.children.val.data() + b);
    }
    if (!CHECK(same)) std::printf("      temoin %s : foret differente\n", w.name);
    CHECK(same_forest(f, kruskal_lots(w.data)));
    CHECK(tower::validate_forests(r.forests.value(), budget).ok());
    // Registre : la cellule de chaque evenement (hyperaretes retenues), croissante dans l'ordre de traitement.
    CHECK_EQ(f.event_cell.size(), nb - 1);
    for (u64 e = 1; e < f.event_cell.size(); ++e) CHECK(f.event_cell[e - 1] <= f.event_cell[e]);
    if (std::string_view(w.name) == "cycle_tri_eq") {
      CHECK(f.event_cell[0] == 0 && f.event_cell[1] == 1);  // la troisieme cellule ferme le cycle sans union
      CHECK_EQ(r.ledger.work[0].retained_cells, 2);
    }
    if (std::string_view(w.name) == "reg238_etoile") CHECK_EQ(r.ledger.work[0].retained_cells, 4);
  }
}

// Union par taille : la chaine qui ferait perdre la grande composante a chaque pas sous une union par indice garde
// une profondeur d'attache au plus floor(log2(naissances)).
MHGP12_TEST(attache, 6) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = pool_of(1);
  const u32 nb = 64;
  OrderData d;
  d.births(nb);
  for (u32 i = nb - 1; i-- > 0;) d.cell(nb - 1 - i, {i, i + 1});
  const Cloud cloud = line_cloud(nb, budget);
  Run r = run(cloud, {d}, budget, *pool);
  REQUIRE(r.forests.ok());
  CHECK(r.ledger.work[0].max_attach_depth <= 6);
  CHECK(tower::validate_forests(r.forests.value(), budget).ok());
  CHECK(same_forest(r.forests.value().orders[0], kruskal_lots(d)));
  CHECK_EQ(r.ledger.work[0].attaches, nb - 1);
  // La meme foret (LEM-T4 : la regle d'union n'y change rien) ; la profondeur, elle, est un compteur du noyau.
  CHECK_EQ(r.forests.value().orders[0].nodes(), 2 * nb - 1);
  CHECK_EQ(r.ledger.object[0].merges, nb - 1);
}

// Domaine des operandes (CST-0212) : 2^31 - 1 naissances admises par le controle de domaine, 2^31 refusees
// (tower_capacity) avant toute lecture des cles et toute allocation ; de meme pour les cellules.
MHGP12_TEST(domaine, 9) {
  const u64 limit = tower::kMaxOrderItems;
  CHECK(tower::operand_domain(limit));
  CHECK(!tower::operand_domain(limit + 1));
  const u64 bytes = (limit + 1) * sizeof(u32);
  void* region = ::mmap(nullptr, bytes, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
  REQUIRE(region != MAP_FAILED);
  const u32* zeros = static_cast<const u32*>(region);
  const std::vector<u64> offsets{0};
  const auto* rank_zeros = reinterpret_cast<const LevelRank*>(region);
  const auto* ball_zeros = reinterpret_cast<const BallIdx*>(region);
  ForestInput in{1, {zeros, limit + 1}, {rank_zeros, limit + 1}, {}, {}, offsets, {}};
  CHECK_EQ(tower::check_forest_input(in).reason, Reason::tower_capacity);
  in.birth_key = {zeros, limit};
  in.birth_rank = {rank_zeros, limit};
  CHECK_EQ(tower::check_forest_input(in).reason, Reason::tower_invariant);  // cles nulles : domaine admis
  in.birth_key = {zeros, 1};
  in.birth_rank = {rank_zeros, 1};
  in.cell_ball = {ball_zeros, limit + 1};
  in.cell_rank = {rank_zeros, limit + 1};
  CHECK_EQ(tower::check_forest_input(in).reason, Reason::tower_capacity);
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = pool_of(1);
  const Cloud cloud = line_cloud(1, budget);
  const u64 before = budget.used();
  budget.restart_peak();
  in.birth_key = {zeros, limit + 1};
  in.birth_rank = {rank_zeros, limit + 1};
  in.cell_ball = {};
  in.cell_rank = {};
  const std::vector<ForestInput> inputs{in};
  auto refused = tower::build_forests(cloud, {}, inputs, {}, budget, *pool);
  CHECK_EQ(refused.outcome().reason, Reason::tower_capacity);
  CHECK_EQ(budget.used(), before);
  CHECK_EQ(budget.peak(), before);
  ::munmap(region, bytes);
}

std::array<u32, 4> square_support(const void*, u32 ball) noexcept {
  static constexpr std::array<std::array<u32, 4>, 5> kSupports = {{{0, 1, kNone, kNone}, {0, 2, kNone, kNone},
                                                                   {1, 3, kNone, kNone}, {2, 3, kNone, kNone},
                                                                   {0, 3, kNone, kNone}}};
  return ball < kSupports.size() ? kSupports[ball] : std::array<u32, 4>{kNone, kNone, kNone, kNone};
}

// Carre K1..4 (CST-0214) : A=(0,0,0), B=(2,0,0), D=(0,2,0), C=(2,2,0) (identifiants de Morton 0..3) ; quatre boules
// diametrales des cotes au rang 1, cercle (S* = AC) au rang 2, inerte a l'ordre 1.
std::vector<OrderData> square_orders() {
  std::vector<OrderData> orders(4);
  for (u32 k = 1; k <= 4; ++k) orders[k - 1].k = static_cast<Order>(k);
  orders[0].births(4, 0);
  for (u32 ball = 0; ball < 4; ++ball) {
    const auto s = square_support(nullptr, ball);
    orders[0].cell(1, {s[0], s[1]}, ball);
  }
  orders[0].cell(2, {0}, 4);
  orders[1].births(4, 1);
  orders[1].cell(2, {0, 1, 2, 3}, 4);
  for (u32 k = 3; k <= 4; ++k) {
    orders[k - 1].birth_key = {4};
    orders[k - 1].birth_rank = ranks({2});
  }
  return orders;
}

MHGP12_TEST(verticales, 24) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = pool_of(3);
  const std::vector<u32> x{0, 2, 0, 2}, y{0, 0, 2, 2}, z{0, 0, 0, 0};
  const std::vector<PointId> ids{make_id<PointId>(0), make_id<PointId>(1), make_id<PointId>(2), make_id<PointId>(3)};
  auto cloud = prepare_cloud(x, y, z, ids, CoordWidth(), budget);
  REQUIRE(cloud.ok());
  const tower::BallSource balls{nullptr, 5, square_support};
  Run r = run(cloud.value(), square_orders(), budget, *pool, balls);
  REQUIRE(r.forests.ok());
  const TowerForests& t = r.forests.value();
  CHECK(tower::validate_forests(t, budget).ok());
  // Naissances de l'ordre 2 : centres (1,0,0), (0,1,0), (2,1,0), (1,2,0) des boules 0..3 -> noeuds 1, 0, 3, 2.
  const std::vector<u32> canonical{1, 0, 3, 2};
  for (u32 i = 0; i < 4; ++i) CHECK_EQ(t.orders[1].birth_node[i], canonical[i]);
  CHECK_EQ(t.orders[0].nodes(), 5);
  CHECK_EQ(t.orders[1].nodes(), 5);
  for (u32 v = 0; v < 5; ++v) CHECK_EQ(t.orders[1].lower[v], 4);
  CHECK_EQ(t.orders[2].lower[0], 4);  // jonction du cercle a l'ordre 2
  CHECK_EQ(t.orders[3].lower[0], 0);  // naissance du cercle a l'ordre 3 (branche etendue de LEM-T6)
  CHECK_EQ(r.ledger.work[3].t6_from_birth, 1);
  CHECK_EQ(r.ledger.work[1].t6_from_cell, 4);
  // Remontee d'un cran : la cellule de la boule 1 (rang 2) ne fait aucune union ; son sommet est la fusion de rang 1,
  // absorbee au rang 2 par la cellule de la boule 2. La naissance de la boule 1 a l'ordre 2 a pour image ce parent.
  std::vector<OrderData> climb(2);
  climb[0].births(3);
  climb[0].cell(1, {0, 1}, 0);
  climb[0].cell(2, {cell_target(0), 1}, 1);
  climb[0].cell(2, {1, 2}, 2);
  climb[1].k = 2;
  climb[1].birth_key = {1};
  climb[1].birth_rank = ranks({2});
  const Cloud line = line_cloud(3, budget);
  Run c = run(line, climb, budget, *pool);
  REQUIRE(c.forests.ok());
  CHECK_EQ(c.forests.value().orders[1].lower[0], 4);
  CHECK_EQ(c.ledger.work[1].t6_climbs, 1);
  CHECK(tower::validate_forests(c.forests.value(), budget).ok());
  // Requete de LEM-T5 hors domaine (CST-0105) : naissance de rang 1, coupe de rang 0.
  CHECK_EQ(tower::component_at(t.orders[1], 0, 0).outcome().reason, Reason::tower_query_domain);
  auto inside = tower::component_at(t.orders[1], 0, 2);
  CHECK(inside.ok() && inside.value() == 4);
  auto alone = tower::component_at(t.orders[1], 2, 1);
  CHECK(alone.ok() && alone.value() == 2);
}

// Jonction catalogue de T1 -> T, M, V, R et export sur le carre K1..4 : supports et niveaux lus en place dans le
// Catalogue (catalogue_balls, levels), cellules dans l'ordre publie de T1 (S* par positions : AD avant AB). Meme
// registre et memes octets que l'adaptateur a la main dans l'ordre de Morton : les formes non reduites des niveaux
// et les denominateurs des centres sont ici les memes dans les deux ordres.
MHGP12_TEST(catalogue, 30) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = pool_of(2);
  const std::vector<u32> x{0, 2, 0, 2}, y{0, 0, 2, 2}, z{0, 0, 0, 0};
  const std::vector<PointId> ids{make_id<PointId>(0), make_id<PointId>(1), make_id<PointId>(2), make_id<PointId>(3)};
  auto cloud = prepare_cloud(x, y, z, ids, CoordWidth(), budget);
  REQUIRE(cloud.ok());
  CatalogueParams params;
  params.kmax = 4;
  auto built = build_catalogue(cloud.value(), params, budget, *pool);
  REQUIRE(built.ok());
  const Catalogue& cat = built.value();
  auto ball_of = [&cat](u32 a, u32 b) {
    const std::array<SiteIdx, 2> s{make_id<SiteIdx>(a), make_id<SiteIdx>(b)};
    const auto found = cat.find_support(s);
    return found ? idx(*found) : kNone;
  };
  // Boules : cotes AB, AD, BC, DC (rang 1), cercle de S* = AC (rang 2).
  const std::array<u32, 4> sides{ball_of(0, 1), ball_of(0, 2), ball_of(1, 3), ball_of(2, 3)};
  const u32 circle = ball_of(0, 3);
  REQUIRE(cat.balls() == 5 && circle != kNone);
  for (u32 b : sides) CHECK(b != kNone && idx(cat.balls_data()[b].rank) == 1);
  CHECK(ball_of(0, 2) < ball_of(0, 1));  // ordre publie de T1 : positions de S*
  const tower::BallSource source = tower::catalogue_balls(cat);
  for (u32 b = 0; b < cat.balls(); ++b) {
    const auto s = source.support(source.context, b);
    for (u32 j = 0; j < 4; ++j) CHECK_EQ(s[j], idx(cat.balls_data()[b].support[j]));
  }
  // Entrees K1..4 dans l'ordre des BallIdx du catalogue.
  std::vector<OrderData> orders(4);
  for (u32 k = 1; k <= 4; ++k) orders[k - 1].k = static_cast<Order>(k);
  orders[0].births(4, 0);
  std::vector<std::pair<u32, std::vector<u32>>> cells;
  for (u32 b : sides) {
    const auto s = source.support(source.context, b);
    cells.push_back({b, {s[0], s[1]}});
  }
  std::sort(cells.begin(), cells.end());
  std::vector<u32> side_order;
  for (auto& [b, reps] : cells) {
    orders[0].cell(1, reps, b);
    side_order.push_back(b);
    orders[1].birth_key.push_back(b);
    orders[1].birth_rank.push_back(make_id<LevelRank>(1));
  }
  orders[0].cell(2, {0}, circle);
  orders[1].cell(2, {0, 1, 2, 3}, circle);
  for (u32 k = 3; k <= 4; ++k) {
    orders[k - 1].birth_key = {circle};
    orders[k - 1].birth_rank = ranks({2});
  }
  Run r = run(cloud.value(), orders, budget, *pool, source);
  REQUIRE(r.forests.ok());
  const TowerForests& t = r.forests.value();
  CHECK(tower::validate_forests(t, budget).ok());
  for (u32 v = 0; v < 5; ++v) CHECK_EQ(t.orders[1].lower[v], 4);
  CHECK_EQ(t.orders[2].lower[0], 4);
  CHECK_EQ(t.orders[3].lower[0], 0);
  // Centres des naissances de l'ordre 2 : AD (0,1,0), AB (1,0,0), DC (1,2,0), BC (2,1,0) -> noeuds 0, 1, 2, 3.
  CHECK_EQ(t.orders[1].birth_key[0], ball_of(0, 2));
  CHECK_EQ(t.orders[1].birth_key[3], ball_of(1, 3));
  const tower::FullSource from_catalogue{&cloud.value(), cat.levels(), source, &t};
  auto digest = tower::full_digest(from_catalogue);
  REQUIRE(digest.ok());
  // Le meme objet par l'adaptateur a la main (ordre de Morton) : memes octets.
  const tower::BallSource hand{nullptr, 5, square_support};
  Run h = run(cloud.value(), square_orders(), budget, *pool, hand);
  REQUIRE(h.forests.ok());
  const auto pa = num::Point::make(0, 0, 0).value(), pb = num::Point::make(2, 0, 0).value();
  const auto pc = num::Point::make(2, 2, 0).value();
  const std::vector<num::Level> levels{num::Level{}, (*num::Sphere::through(pa, pb).value()).level(),
                                       (*num::Sphere::through(pa, pc).value()).level()};
  const tower::FullSource from_hand{&cloud.value(), levels, hand, &h.forests.value()};
  auto other = tower::full_digest(from_hand);
  REQUIRE(other.ok());
  CHECK(digest.value() == other.value());
  CHECK_EQ(cat.levels().size(), 3);
}

// Branches ouvertes des hyperaretes retenues (registre R ; recu de l'auditeur Codex audit_t1b_tour_prepublication) :
// trois naissances et deux cellules de rang 1, {0,1} puis {0,2} ou {1,2} : memes evenements, meme foret, branches de la
// seconde cellule {0,2} ou {1,2} ; sept naissances, cellules {0,1}, {0,1,2}, ..., {0,..,6} au meme rang : six
// evenements, six cellules retenues, 27 branches (A non borne par naissances - 1) ; hypergraphes aleatoires contre la
// coupe ouverte de la foret de reference (remontee parent par parent) et un union-find independant des retenues.
MHGP12_TEST(branches, 330) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = pool_of(3);
  const Cloud cloud3 = line_cloud(3, budget);
  for (const u32 second : {0u, 1u}) {
    OrderData d;
    d.births(3);
    d.cell(1, {0, 1});
    d.cell(1, {second, 2});
    Run r = run(cloud3, {d}, budget, *pool);
    REQUIRE(r.forests.ok());
    const OrderForest& f = r.forests.value().orders[0];
    CHECK_EQ(f.retained_cell.size(), 2);
    CHECK_EQ(f.branches.off[1], 2);
    CHECK(f.branches.val[0] == 0 && f.branches.val[1] == 1);
    CHECK(f.branches.val[2] == second && f.branches.val[3] == 2);  // coupe ouverte : {0,2} ou {1,2}
    CHECK_EQ(f.nodes(), 4);                                         // une seule fusion ternaire
    CHECK(tower::validate_forests(r.forests.value(), budget).ok());
  }
  const Cloud cloud7 = line_cloud(7, budget);
  OrderData d7;
  d7.births(7);
  for (u32 n = 2; n <= 7; ++n) {
    std::vector<u32> targets(n);
    std::iota(targets.begin(), targets.end(), 0u);
    d7.cell(1, targets);
  }
  Run r7 = run(cloud7, {d7}, budget, *pool);
  REQUIRE(r7.forests.ok());
  CHECK_EQ(r7.ledger.work[0].events, 6);
  CHECK_EQ(r7.ledger.work[0].retained_cells, 6);
  CHECK_EQ(r7.ledger.work[0].branches, 27);
  CHECK_EQ(r7.forests.value().orders[0].branches.val.size(), 27);
  CHECK_EQ(check_rows(d7, r7.forests.value().orders[0]), 0);
  std::mt19937_64 rng(31);
  u64 rows = 0;
  for (int it = 0; it < 300; ++it) {
    const u32 nb = 2 + static_cast<u32>(rng() % 30);
    const OrderData d = random_order(rng, nb, 2 * nb);
    const Cloud cloud = line_cloud(nb, budget);
    tower::ForestParams params;
    params.slice_events = 1 + static_cast<u32>(rng() % 4);
    Run r = run(cloud, {d}, budget, *pool, {}, params);
    REQUIRE(r.forests.ok());
    CHECK_EQ(check_rows(d, r.forests.value().orders[0]), 0);
    rows += r.forests.value().orders[0].retained_cell.size();
  }
  CHECK(rows >= 3000);
  std::printf("branches lignes=%llu\n", static_cast<unsigned long long>(rows));
}

MHGP12_TEST(requetes, 2000) {
  // component_at contre la remontee parent par parent, sur des forets aleatoires a toutes les coupes.
  std::mt19937_64 rng(7);
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = pool_of(2);
  u64 queries = 0;
  for (int it = 0; it < 60; ++it) {
    const u32 nb = 2 + static_cast<u32>(rng() % 30);
    const OrderData d = random_order(rng, nb, 2 * nb);
    const Cloud cloud = line_cloud(nb, budget);
    Run r = run(cloud, {d}, budget, *pool);
    REQUIRE(r.forests.ok());
    const OrderForest& f = r.forests.value().orders[0];
    const u32 top = f.rank[f.root] + 1;
    for (u32 leaf = 0; leaf < nb; ++leaf)
      for (u32 rank = 0; rank <= top; ++rank) {
        u32 v = leaf;
        while (f.parent[v] != kNone && f.rank[f.parent[v]] <= rank) v = f.parent[v];
        auto got = tower::component_at(f, leaf, rank);
        CHECK(got.ok() && got.value() == v);
        ++queries;
      }
  }
  CHECK(queries >= 2000);
}

MHGP12_TEST(determinisme, 30) {
  std::mt19937_64 rng(11);
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const u32 nb = 4000;
  const OrderData d = random_order(rng, nb, 3 * nb);
  const Cloud cloud = line_cloud(nb, budget);
  std::vector<Run> runs;
  for (const u32 threads : {1u, 3u, 8u})
    for (const u32 slice : {1u, 97u, 1u << 16}) {
      auto pool = pool_of(threads);
      tower::ForestParams params;
      params.slice_events = slice;
      runs.push_back(run(cloud, {d}, budget, *pool, {}, params));
      REQUIRE(runs.back().forests.ok());
    }
  for (const Run& r : runs) {
    CHECK(same_registry(r.forests.value().orders[0], runs[0].forests.value().orders[0]));
    CHECK(r.ledger.work[0] == runs[0].ledger.work[0]);
    CHECK(r.ledger.object[0] == runs[0].ledger.object[0]);
  }
  CHECK(runs.back().ledger.slices == 1 && runs.front().ledger.slices > 1);
}

MHGP12_TEST(refus, 20) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = pool_of(2);
  const Cloud cloud = line_cloud(3, budget);
  const u64 base = budget.used();
  auto refused = [&](const OrderData& d) {
    Reason reason = Reason::none;
    {
      Run r = run(cloud, {d}, budget, *pool);
      if (!r.forests.ok()) {
        reason = r.forests.outcome().reason;
        CHECK_EQ(budget.used(), base);  // transactionnel : un refus ne retient rien
      }
    }
    CHECK_EQ(budget.used(), base);
    return reason;
  };
  OrderData d;
  d.births(3);
  d.cell(1, {0, 1});
  d.cell(2, {1, 2});
  CHECK_EQ(refused(d), Reason::none);
  OrderData outside = d;
  outside.targets[0] = 3;  // naissance hors de la liste
  CHECK_EQ(refused(outside), Reason::tower_invariant);
  OrderData same_rank = d;
  same_rank.cell_rank[1] = make_id<LevelRank>(1);
  same_rank.targets[2] = cell_target(0);  // cellule de meme rang : pas encore traitee au sens de LEM-T3
  CHECK_EQ(refused(same_rank), Reason::tower_invariant);
  OrderData late = d;
  late.birth_rank = ranks({0, 0, 2});  // naissance de rang egal a celui de sa cellule
  CHECK_EQ(refused(late), Reason::tower_invariant);
  OrderData two_roots;
  two_roots.births(3);
  two_roots.cell(1, {0, 1});
  CHECK_EQ(refused(two_roots), Reason::tower_invariant);
  OrderData sentinel = d;
  sentinel.targets[1] = kNoTarget;
  CHECK_EQ(refused(sentinel), Reason::tower_invariant);
  OrderData empty_cell = d;
  empty_cell.rep_offsets = {0, 0, 4};
  CHECK_EQ(refused(empty_cell), Reason::tower_invariant);
}

// Admission par etage (regle du socle, core/buffer.hpp ; recu audit_registre_branches_20261007 : decalages de la CSR
// des branches alloues sans admission). Les etages T, M, V et R sont joues un par un sur l'etat interne de
// build_forests, budget sans cache (tailles exactes) : le pic mesure de chacun, au-dessus de l'usage a son entree, ne
// depasse pas les octets qu'il a admis. Nuages par la chaine du produit (index, catalogue de T1, resolve_tower), K de
// 2 a 5, sites tires dans un cube de 2^16 de cote ; registre identique a celui de build_forests.
MHGP12_TEST(admission, 170) {
  std::mt19937_64 rng(20261008);
  auto pool = pool_of(3);
  using Stage = Outcome (*)(tower::detail::BuildState&, sched::Pool&) noexcept;
  const std::array<Stage, tower::detail::kStages> stages{tower::detail::run_kernels, tower::detail::run_contraction,
                                                         tower::detail::run_verticals, tower::detail::run_registry};
  u64 played = 0, upper = 0, rows = 0;
  for (int it = 0; it < 40; ++it) {
    MemoryBudget budget(MemoryBudget::kUnlimited);
    const u32 n = 12 + static_cast<u32>(rng() % 150);
    std::vector<u32> x(n), y(n), z(n);
    std::vector<PointId> ids(n);
    for (u32 i = 0; i < n; ++i) {
      x[i] = static_cast<u32>(rng() % 65536);
      y[i] = static_cast<u32>(rng() % 65536);
      z[i] = static_cast<u32>(rng() % 65536);
      ids[i] = make_id<PointId>(i);
    }
    auto cloud = prepare_cloud(x, y, z, ids, CoordWidth(), budget);
    REQUIRE(cloud.ok());
    auto index = build_index(std::move(cloud).take(), IndexParams{}, budget);
    REQUIRE(index.ok());
    CatalogueParams params;
    params.kmax = 2 + static_cast<int>(rng() % 4);
    auto catalogue = build_catalogue(index.value().cloud(), params, budget, *pool);
    REQUIRE(catalogue.ok());
    auto resolution = resolve_tower(index.value(), catalogue.value(), budget, *pool);
    REQUIRE(resolution.ok());
    std::vector<ForestInput> inputs;
    for (Order k = 1; k <= resolution.value().orders(); ++k)
      inputs.push_back(tower::forest_input(resolution.value().order(k)));
    const tower::BallSource balls = tower::catalogue_balls(catalogue.value());
    const tower::ForestParams forest_params;
    auto reference = tower::build_forests(index.value().cloud(), balls, inputs, forest_params, budget, *pool);
    REQUIRE(reference.ok());
    {
      auto state = std::make_unique<tower::detail::BuildState>(index.value().cloud(), balls, inputs, forest_params,
                                                               budget);
      tower::detail::BuildState& s = *state;
      s.forests.kmax = static_cast<Order>(inputs.size());
      for (u32 stage = 0; stage < tower::detail::kStages; ++stage) {
        const u64 before = budget.used();
        budget.restart_peak();
        REQUIRE(stages[stage](s, *pool).ok());
        const u64 rise = budget.restart_peak() - before;
        CHECK(rise <= s.admitted[stage]);
        if (rise > s.admitted[stage])
          std::printf("admission etage=%u pic=%llu admis=%llu\n", stage, static_cast<unsigned long long>(rise),
                      static_cast<unsigned long long>(s.admitted[stage]));
        upper += rise == s.admitted[stage];
        ++played;
      }
      for (u64 i = 0; i < inputs.size(); ++i) {
        CHECK(same_registry(s.forests.orders[i], reference.value().orders[i]));
        rows += s.forests.orders[i].retained_cell.size();
      }
    }
    CHECK(tower::validate_forests(reference.value(), budget).ok());
  }
  CHECK_EQ(played, 160);
  CHECK(rows >= 1000);
  std::printf("admission etages=%llu egalites=%llu lignes=%llu\n", static_cast<unsigned long long>(played),
              static_cast<unsigned long long>(upper), static_cast<unsigned long long>(rows));
}

}  // namespace
}  // namespace mhgp12::tower_test

MHGP12_TEST_MAIN()
