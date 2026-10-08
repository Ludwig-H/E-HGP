// Portes du raccourci du registre R (levier R1 : critere q = d + 1 et preuve du recu de l'auditeur Codex
// registre_classe_unique ; protocole de registre_classe_unique_patch), hors produit. Pour chaque ligne retenue, la
// porte decide SANS le critere du produit si sa cellule est la seule contributrice de sa classe M (cellules retenues
// du meme rang dont les feuilles tombent dans le meme noeud de la foret de reference kruskal_lots), d'ou les
// representants a relire attendus Q_g (lignes generales seulement), Q_R (toutes les lignes) et les branches A ; les
// branches de chaque ligne sont jugees contre la coupe ouverte de la foret de reference (check_rows). Groupes :
//   fixtures   les neuf fixtures du modele de l'auditeur (model.py, cas fermes par une cellule finale) : lignes,
//              branches, requetes evitees Q_R - Q_g et lignes generales egales aux valeurs du modele ; sous-aretes
//              partagees ({0,1} puis {0,2}) et inclusions ({0,1} puis {0,1,2}) au meme rang : voie generale ; grande
//              cellule a doublons puis cellule inerte : tout direct (Q_g = 0) ;
//   exhaustif  les 147 cas bornes du modele (deux cellules sur trois naissances, rangs (1,1), (1,2), (2,2), cellule de
//              fermeture) ;
//   aleatoire  hypergraphes aleatoires (doublons, cibles cellule) a 1, 3 et 8 fils et plusieurs tranches de M :
//              branches, Q_g et A attendus, registre identique d'un nombre de fils a l'autre ;
//   grand      un ordre de plus de 2 048 lignes (plusieurs morceaux de R), lignes directes et generales melees ;
//   etapes     prepare_rows rend Q_g et n'alloue branch_nodes que si Q_g > 0 (taille Q_g) ; passes de R jouees une a
//              une (collect, place, fill) : memes branches et compteurs que build_forests ; etage R complet : pic au
//              plus ses octets admis (premiere admission en Q_g), sur des cas a lignes generales.
#include <map>
#include <random>

#include "forest_support.hpp"
#include "tower/forest_internal.hpp"

namespace mhgp12::tower_test {
namespace {

// Attendus independants d'un ordre : lignes, lignes generales, representants des lignes (Q_R) et des seules lignes
// generales (Q_g), branches (A).
struct Expect {
  u64 rows = 0, general = 0, q_r = 0, q_g = 0, a = 0;
};

Expect expect(const OrderData& d) {
  const u32 nb = static_cast<u32>(d.birth_key.size());
  const Reference ref = kruskal_lots(d);
  std::vector<u32> dsu(nb);
  std::iota(dsu.begin(), dsu.end(), 0u);
  auto find = [&dsu](u32 x) {
    while (dsu[x] != x) x = dsu[x] = dsu[dsu[x]];
    return x;
  };
  struct Row {
    u32 node;
    u64 reps, branches;
  };
  std::vector<Row> rows;
  std::map<u32, u64> owners;  // classe (noeud de reference de rang r) -> cellules retenues qui y tombent
  for (u64 t = 0; t < d.cell_ball.size(); ++t) {
    const u32 r = idx(d.cell_rank[t]);
    std::vector<u32> leaves, open;
    for (u64 p = d.rep_offsets[t]; p < d.rep_offsets[t + 1]; ++p) leaves.push_back(reference_leaf(d, d.targets[p]));
    bool retained = false;
    for (u32 l : leaves) {
      const u32 x = find(leaves[0]), y = find(l);
      if (x != y) dsu[y] = x, retained = true;
    }
    if (!retained) continue;
    for (u32 l : leaves) open.push_back(open_cut(ref, l, r - 1));
    std::sort(open.begin(), open.end());
    const u64 distinct = static_cast<u64>(std::unique(open.begin(), open.end()) - open.begin());
    const u32 node = open_cut(ref, leaves[0], r);  // la cellule unit au rang r : noeud de rang r de sa classe
    ++owners[node];
    rows.push_back({node, leaves.size(), distinct});
  }
  Expect e;
  for (const Row& row : rows) {
    const bool general = owners[row.node] > 1;
    ++e.rows;
    e.general += general;
    e.q_r += row.reps;
    e.q_g += general ? row.reps : 0;
    e.a += row.branches;
  }
  return e;
}

// Un ordre par build_forests ; controles independants des lignes et des compteurs de R. Rend l'attendu.
Expect judge(const OrderData& d, sched::Pool& pool, tower::ForestParams params = {}) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const Cloud cloud = line_cloud(static_cast<u32>(d.birth_key.size()), budget);
  const Expect e = expect(d);
  Run r = run(cloud, {d}, budget, pool, {}, params);
  if (!CHECK(r.forests.ok())) return e;
  const OrderForest& f = r.forests.value().orders[0];
  CHECK_EQ(check_rows(d, f), 0);
  CHECK_EQ(f.retained_cell.size(), e.rows);
  CHECK_EQ(f.branches.val.size(), e.a);
  CHECK_EQ(r.ledger.work[0].branches, e.a);
  CHECK_EQ(r.ledger.work[0].branch_reads, e.q_g);  // representants reellement relus : lignes generales seulement
  return e;
}

// Fixture du modele : dates des naissances, cellules (rang, cibles), fermee par une cellule finale qui relie tout.
OrderData fixture(std::initializer_list<u32> dates, const std::vector<std::pair<u32, std::vector<u32>>>& cells) {
  OrderData d;
  u32 top = 0;
  for (u32 date : dates) {
    d.births(1, date);
    top = std::max(top, date);
  }
  for (const auto& [rank, targets] : cells) {
    d.cell(rank, targets);
    top = std::max(top, rank);
  }
  std::vector<u32> all(dates.size());
  std::iota(all.begin(), all.end(), 0u);
  d.cell(top + 1, all);
  return d;
}

struct Model {
  const char* name;
  OrderData data;
  u64 rows, general, saved, values;  // valeurs de model.py (lignes, lignes generales, requetes evitees, branches)
};

std::vector<Model> models() {
  const u32 c0 = cell_target(0), c1 = cell_target(1), c2 = cell_target(2);
  using Cells = std::vector<std::pair<u32, std::vector<u32>>>;
  std::vector<Model> out;
  out.push_back({"singleton", fixture({0}, {}), 0, 0, 0, 0});
  out.push_back({"deux_classes_meme_plateau", fixture({0, 0, 0, 0}, Cells{{1, {0, 1}}, {1, {2, 3}}}), 3, 0, 8, 6});
  out.push_back({"sous_aretes_partagees", fixture({0, 0, 0}, Cells{{1, {0, 1}}, {1, {0, 2}}}), 2, 2, 0, 4});
  out.push_back({"sous_aretes_inclusions", fixture({0, 0, 0}, Cells{{1, {0, 1}}, {1, {0, 1, 2}}}), 2, 2, 0, 5});
  out.push_back({"grande_cellule_et_inerte",
                 fixture({0, 0, 0, 0, 0, 0}, Cells{{1, {0, 1, 2, 3, 4, 5, 0, 2}}, {1, {0, 2}}}), 1, 0, 8, 6});
  out.push_back({"ancrage_cellule_inerte", fixture({0, 0, 0}, Cells{{1, {1}}, {1, {0, 1}}, {2, {c0, 2}}}), 2, 0, 4, 4});
  out.push_back({"cibles_cellules_et_doublons",
                 fixture({0, 0, 0, 0, 0}, Cells{{1, {2, 1}}, {1, {1, 0}}, {2, {c0, c1, 3, 3}}, {3, {c2, 4}}}), 4, 2, 6,
                 8});
  out.push_back({"naissances_datees", fixture({0, 0, 2, 3}, Cells{{1, {0, 1}}, {3, {c0, 2}}, {4, {c1, 3}}}), 3, 0, 6,
                 6});
  Cells prefixes;
  for (u32 n = 2; n <= 7; ++n) {
    std::vector<u32> targets(n);
    std::iota(targets.begin(), targets.end(), 0u);
    prefixes.push_back({1, targets});
  }
  out.push_back({"branches_non_bornees_par_evenements", fixture({0, 0, 0, 0, 0, 0, 0}, prefixes), 6, 6, 0, 27});
  return out;
}

MHGP12_TEST(fixtures, 63) {
  auto pool = pool_of(3);
  for (const Model& m : models()) {
    const Expect e = judge(m.data, *pool);
    const bool same = e.rows == m.rows && e.general == m.general && e.q_r - e.q_g == m.saved && e.a == m.values;
    CHECK(same);
    std::printf("fixture %s lignes=%llu generales=%llu Q_R=%llu Q_g=%llu A=%llu\n", m.name,
                static_cast<unsigned long long>(e.rows), static_cast<unsigned long long>(e.general),
                static_cast<unsigned long long>(e.q_r), static_cast<unsigned long long>(e.q_g),
                static_cast<unsigned long long>(e.a));
  }
}

MHGP12_TEST(exhaustif, 880) {
  auto pool = pool_of(2);
  std::vector<std::vector<u32>> subsets;
  for (u32 mask = 1; mask < 8; ++mask) {
    std::vector<u32> s;
    for (u32 b = 0; b < 3; ++b)
      if (mask & (1u << b)) s.push_back(b);
    subsets.push_back(s);
  }
  // ordre de model.py : tailles croissantes, puis ordre lexicographique
  std::stable_sort(subsets.begin(), subsets.end(), [](const auto& a, const auto& b) { return a.size() < b.size(); });
  u64 cases = 0, general = 0, direct = 0;
  for (const auto& left : subsets)
    for (const auto& right : subsets)
      for (const auto& [r, s] : {std::pair<u32, u32>{1, 1}, {1, 2}, {2, 2}}) {
        const Expect e = judge(fixture({0, 0, 0}, {{r, left}, {s, right}}), *pool);
        general += e.general;
        direct += e.rows - e.general;
        ++cases;
      }
  CHECK_EQ(cases, 147);
  CHECK(general >= 10 && direct >= 100);
  std::printf("exhaustif cas=%llu lignes_directes=%llu lignes_generales=%llu\n", static_cast<unsigned long long>(cases),
              static_cast<unsigned long long>(direct), static_cast<unsigned long long>(general));
}

MHGP12_TEST(aleatoire, 3000) {
  std::mt19937_64 rng(20261008);
  std::array<std::unique_ptr<sched::Pool>, 3> pools{pool_of(1), pool_of(3), pool_of(8)};
  u64 general = 0, direct = 0, cases = 0;
  for (int it = 0; it < 200; ++it) {
    const u32 nb = 2 + static_cast<u32>(rng() % 40);
    const OrderData d = random_order(rng, nb, 1 + static_cast<u32>(rng() % (3 * nb)));
    tower::ForestParams params;
    params.slice_events = 1 + static_cast<u32>(rng() % 4);
    MemoryBudget budget(MemoryBudget::kUnlimited);
    const Cloud cloud = line_cloud(nb, budget);
    std::array<Run, 3> runs{run(cloud, {d}, budget, *pools[0], {}, params),
                            run(cloud, {d}, budget, *pools[1], {}, params),
                            run(cloud, {d}, budget, *pools[2], {}, params)};
    const Expect e = judge(d, *pools[1], params);
    for (const Run& r : runs) {
      if (!CHECK(r.forests.ok())) continue;
      CHECK(same_registry(r.forests.value().orders[0], runs[0].forests.value().orders[0]));
      CHECK(r.ledger.work[0] == runs[0].ledger.work[0]);
    }
    general += e.general;
    direct += e.rows - e.general;
    ++cases;
  }
  CHECK(general >= 1200 && direct >= 400);
  std::printf("aleatoire ordres=%llu lignes_directes=%llu lignes_generales=%llu\n",
              static_cast<unsigned long long>(cases), static_cast<unsigned long long>(direct),
              static_cast<unsigned long long>(general));
}

// Plus de 2 048 lignes : paires {2i, 2i+1} au rang 1 (directes), puis au rang 2 des triplets de paires reunis par deux
// cellules (une classe a deux contributrices : lignes generales) ou par une seule (directe), fermeture au rang 3.
OrderData large_order(u32 groups) {
  OrderData d;
  d.births(6 * groups);
  for (u32 i = 0; i < 3 * groups; ++i) d.cell(1, {2 * i, 2 * i + 1});
  for (u32 g = 0; g < groups; ++g) {
    const u32 a = 6 * g, b = a + 2, c = a + 4;
    if (g % 2 == 0) {
      d.cell(2, {a, b});
      d.cell(2, {b + 1, c});  // meme classe que la precedente : deux contributrices
    } else {
      d.cell(2, {a, b, c + 1});  // seule contributrice : directe
    }
  }
  std::vector<u32> all(6 * groups);
  std::iota(all.begin(), all.end(), 0u);
  d.cell(3, all);
  return d;
}

MHGP12_TEST(grand, 8) {
  auto pool = pool_of(3);
  const OrderData d = large_order(1200);
  const Expect e = judge(d, *pool);
  CHECK(e.rows > 2 * 2048);
  CHECK(e.general >= 1000 && e.rows - e.general >= 3000);
  std::printf("grand lignes=%llu generales=%llu Q_R=%llu Q_g=%llu A=%llu\n", static_cast<unsigned long long>(e.rows),
              static_cast<unsigned long long>(e.general), static_cast<unsigned long long>(e.q_r),
              static_cast<unsigned long long>(e.q_g), static_cast<unsigned long long>(e.a));
}

// Etapes de R une a une sur un etat de build_forests apres T, M et V ; puis l'etage R complet sous mesure du pic. Les
// octets admis par R, moins la part exacte 12 R + 8 (R + 1) + 4 Q_g + 4 R (travail) + 4 A + 8 (R + 1) (sortie), sont
// rendus dans residue (taches et compteurs des morceaux : meme valeur pour toute entree d'un seul morceau) : une
// admission en Q_R au lieu de Q_g ferait varier ce reste d'une entree a l'autre.
void steps(const OrderData& d, sched::Pool& pool, u64& general_cases, std::vector<u64>& residue) {
  const Expect e = expect(d);
  MemoryBudget budget(MemoryBudget::kUnlimited);
  const Cloud cloud = line_cloud(static_cast<u32>(d.birth_key.size()), budget);
  const std::vector<ForestInput> inputs{d.view()};
  const tower::BallSource balls;
  const tower::ForestParams params;
  auto reference = tower::build_forests(cloud, balls, inputs, params, budget, pool);
  REQUIRE(reference.ok());
  for (const bool whole : {false, true}) {
    auto state = std::make_unique<tower::detail::BuildState>(cloud, balls, inputs, params, budget);
    tower::detail::BuildState& s = *state;
    s.forests.kmax = 1;
    REQUIRE(tower::detail::run_kernels(s, pool).ok());
    REQUIRE(tower::detail::run_contraction(s, pool).ok());
    REQUIRE(tower::detail::run_verticals(s, pool).ok());
    if (whole) {  // etage complet : pic au plus les octets admis (premiere admission en Q_g, puis la sortie)
      const u64 before = budget.used();
      budget.restart_peak();
      REQUIRE(tower::detail::run_registry(s, pool).ok());
      const u64 admitted = s.admitted[tower::detail::kStageR];
      CHECK(budget.restart_peak() - before <= admitted);
      general_cases += e.q_g > 0;
      const u64 exact = 12 * e.rows + 8 * (e.rows + 1) + 4 * e.q_g + 4 * e.rows + 4 * e.a + 8 * (e.rows + 1);
      if (e.rows > 0 && e.rows <= 2048) residue.push_back(admitted - exact);
    } else {
      u64 reads = 0;
      REQUIRE(tower::detail::prepare_rows(s, 0, reads).ok());
      CHECK_EQ(reads, e.q_g);
      CHECK_EQ(s.work[0].branch_nodes.size(), e.q_g);  // aucun tampon si tout est direct
      const u64 rows = s.forests.orders[0].retained_cell.size();
      tower::ForestWork counters;
      REQUIRE(tower::detail::collect_rows(s, 0, 0, rows, counters).ok());
      CHECK_EQ(counters.branch_reads, e.q_g);
      CHECK_EQ(counters.branches, e.a);
      REQUIRE(tower::detail::place_rows(s, 0).ok());
      tower::detail::fill_rows(s, 0, 0, rows);
      tower::detail::close_rows(s, 0);
    }
    CHECK(same_registry(s.forests.orders[0], reference.value().orders[0]));
  }
}

MHGP12_TEST(etapes, 542) {
  auto pool = pool_of(3);
  u64 general_cases = 0;
  std::vector<u64> residue;
  for (const Model& m : models()) steps(m.data, *pool, general_cases, residue);
  std::mt19937_64 rng(31);
  for (int it = 0; it < 20; ++it) {
    const u32 nb = 4 + static_cast<u32>(rng() % 30);
    steps(random_order(rng, nb, 2 * nb), *pool, general_cases, residue);
  }
  steps(large_order(600), *pool, general_cases, residue);
  CHECK(general_cases >= 10);
  // premiere admission en Q_g exactement : meme reste pour toutes les entrees d'un seul morceau
  CHECK(residue.size() >= 20 && residue[0] > 0 &&
        std::all_of(residue.begin(), residue.end(), [&](u64 r) { return r == residue[0]; }));
}

}  // namespace
}  // namespace mhgp12::tower_test

MHGP12_TEST_MAIN()
