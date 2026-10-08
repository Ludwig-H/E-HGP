// mhgp12_tower_forest_oracle : adaptateur natif de la porte de l'oracle borne (tests/tower/oracle_tour.py). Lit sur
// l'entree standard des cas en texte (nuage, boules du catalogue de la reference avec leur rang et leur support S*, puis par
// ordre les naissances, les cellules de fenetre non naissances et les cibles de leurs representants rendues par la
// regle de resolution de la v12 de la reference), joue T, M, V et R, valide le registre, et ecrit le registre en
// texte ; avec un dossier, ecrit aussi le vidage FUL1 de chaque cas (<dossier>/<numero>.ful1) pour le lecteur strict.
//
//   mhgp12_tower_forest_oracle [--fils N] [--tranche E] [--vidages <dossier>] < cas.txt > registres.txt
//
// Entree : << nuage NOM POINTS ORDRES BOULES >>, POINTS lignes << p x y z >> (PointId = rang de la ligne), BOULES lignes
// << b RANG s0 [s1 [s2 [s3]]] >>, puis par ordre << ordre k NAISSANCES CELLULES >>, << n CLE RANG >>, << c BOULE RANG
// CIBLE... >> (cible : indice de naissance, ou indice de cellule + 2^31), et << fin >>. Sortie par cas : << cas NOM >>,
// par ordre << ordre k NAISSANCES NOEUDS RACINE >> puis par noeud << v RANG PARENT VERTICALE ENFANT... >> (kNone ecrit
// -1), << compteurs ... >>, puis << statut ok >> ou << statut refus RAISON >>. Code 0 si le texte est lisible (les
// refus sont dans la sortie), 2 sinon.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "io/io.hpp"
#include "sched/sched.hpp"
#include "tower/tower.hpp"

namespace {

using namespace mhgp12;

struct Case {
  std::string name;
  std::vector<u32> x, y, z;
  std::vector<std::array<u32, 4>> supports;
  std::vector<u32> ball_rank;
  struct OrderRows {
    Order k = 0;
    std::vector<u32> birth_key, targets;
    std::vector<LevelRank> birth_rank, cell_rank;
    std::vector<BallIdx> cell_ball;
    std::vector<u64> rep_offsets{0};
  };
  std::vector<OrderRows> orders;
};

std::array<u32, 4> support_of(const void* context, u32 ball) noexcept {
  return static_cast<const Case*>(context)->supports[ball];
}

bool read_case(std::istream& in, Case& c, bool& done) {
  std::string word;
  if (!(in >> word)) return done = true, true;
  u64 points = 0, orders = 0, balls = 0;
  if (word != "nuage" || !(in >> c.name >> points >> orders >> balls)) return false;
  for (u64 i = 0; i < points; ++i) {
    u32 x, y, z;
    if (!(in >> word >> x >> y >> z) || word != "p") return false;
    c.x.push_back(x), c.y.push_back(y), c.z.push_back(z);
  }
  std::string line;
  std::getline(in, line);
  for (u64 b = 0; b < balls; ++b) {
    if (!std::getline(in, line)) return false;
    std::istringstream row(line);
    std::array<u32, 4> s{kNone, kNone, kNone, kNone};
    u32 rank = 0;
    if (!(row >> word >> rank) || word != "b") return false;
    for (u32 j = 0; j < 4 && row >> s[j]; ++j) {
    }
    c.supports.push_back(s);
    c.ball_rank.push_back(rank);
  }
  for (u64 o = 0; o < orders; ++o) {
    Case::OrderRows ord;
    u32 k = 0;
    u64 births = 0, cells = 0;
    if (!(in >> word >> k >> births >> cells) || word != "ordre") return false;
    ord.k = static_cast<Order>(k);
    for (u64 i = 0; i < births; ++i) {
      u32 key, rank;
      if (!(in >> word >> key >> rank) || word != "n") return false;
      ord.birth_key.push_back(key), ord.birth_rank.push_back(make_id<LevelRank>(rank));
    }
    std::getline(in, line);
    for (u64 t = 0; t < cells; ++t) {
      if (!std::getline(in, line)) return false;
      std::istringstream row(line);
      u32 ball, rank;
      u64 target;
      if (!(row >> word >> ball >> rank) || word != "c") return false;
      ord.cell_ball.push_back(make_id<BallIdx>(ball)), ord.cell_rank.push_back(make_id<LevelRank>(rank));
      while (row >> target) ord.targets.push_back(static_cast<u32>(target));
      ord.rep_offsets.push_back(ord.targets.size());
    }
    c.orders.push_back(ord);
  }
  return (in >> word) && word == "fin";
}

// Niveaux par rang : forme non reduite de la premiere boule du rang (rang 0 : niveau nul).
bool case_levels(const Case& c, const Cloud& cloud, std::vector<num::Level>& levels) {
  u32 top = 0;
  for (u32 r : c.ball_rank) top = std::max(top, r);
  levels.assign(u64{top} + 1, num::Level{});
  std::vector<bool> seen(levels.size(), false);
  for (u32 b = 0; b < c.supports.size(); ++b) {
    const u32 r = c.ball_rank[b];
    if (seen[r]) continue;
    seen[r] = true;
    if (c.supports[b][1] == kNone) continue;  // rayon nul
    std::array<num::Point, 4> p{};
    u32 q = 0;
    for (; q < 4 && c.supports[b][q] != kNone; ++q) {
      const u32 s = c.supports[b][q];
      p[q] = num::Point::make(cloud.x()[s], cloud.y()[s], cloud.z()[s]).value();
    }
    auto sphere = q == 2 ? num::Sphere::through(p[0], p[1]) : q == 3 ? num::Sphere::through(p[0], p[1], p[2])
                                                                     : num::Sphere::through(p[0], p[1], p[2], p[3]);
    if (!sphere.ok() || !sphere.value()) return false;
    levels[r] = sphere.value()->level();
  }
  return true;
}

void print_forests(const tower::TowerForests& t, const tower::ForestLedger& l) {
  for (u32 i = 0; i < t.kmax; ++i) {
    const tower::OrderForest& f = t.orders[i];
    std::printf("ordre %u %u %u %u\n", i + 1, f.births, f.nodes(), f.root);
    for (u32 v = 0; v < f.nodes(); ++v) {
      std::printf("v %u %lld %lld", f.rank[v], f.parent[v] == kNone ? -1LL : (long long)f.parent[v],
                  f.lower.empty() ? -1LL : (long long)f.lower[v]);
      for (u64 j = f.children.off[v]; j < f.children.off[u64{v} + 1]; ++j) std::printf(" %u", f.children.val[j]);
      std::printf("\n");
    }
    const tower::ForestWork& w = l.work[i];
    std::printf("compteurs %u cibles_cellule=%llu inertes=%llu remontees=%llu t6_naissance=%llu t5=%llu cohortes=%llu\n",
                i + 1, (unsigned long long)w.cell_targets, (unsigned long long)l.object[i].inert_cells,
                (unsigned long long)w.t6_climbs, (unsigned long long)w.t6_from_birth,
                (unsigned long long)w.t5_queries, (unsigned long long)w.cohorts);
  }
}

int play(const Case& c, sched::Pool& pool, u32 slice, const std::string& dumps, u64 number) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  std::vector<PointId> ids;
  for (u32 i = 0; i < c.x.size(); ++i) ids.push_back(make_id<PointId>(i));
  auto cloud = prepare_cloud(c.x, c.y, c.z, ids, CoordWidth(), budget);
  std::printf("cas %s\n", c.name.c_str());
  if (!cloud.ok()) return std::printf("statut refus %s\n", std::string(reason_name(cloud.outcome().reason)).c_str()), 0;
  std::vector<tower::ForestInput> inputs;
  for (const auto& o : c.orders)
    inputs.push_back(tower::ForestInput{o.k, o.birth_key, o.birth_rank, o.cell_ball, o.cell_rank, o.rep_offsets,
                                        o.targets});
  const tower::BallSource balls{&c, c.supports.size(), support_of};
  tower::ForestParams params;
  params.slice_events = slice;
  tower::ForestLedger ledger;
  auto forests = tower::build_forests(cloud.value(), balls, inputs, params, budget, pool, &ledger);
  if (!forests.ok())
    return std::printf("statut refus %s\n", std::string(reason_name(forests.outcome().reason)).c_str()), 0;
  const Outcome valid = tower::validate_forests(forests.value(), budget);
  if (!valid.ok()) return std::printf("statut refus %s\n", std::string(reason_name(valid.reason)).c_str()), 0;
  print_forests(forests.value(), ledger);
  if (!dumps.empty()) {
    std::vector<num::Level> levels;
    if (!case_levels(c, cloud.value(), levels)) return std::printf("statut refus niveaux\n"), 0;
    const tower::FullSource source{&cloud.value(), levels, balls, &forests.value()};
    auto dir = io::OutputDirectory::plan((dumps + "/" + std::to_string(number)).c_str(), {});
    if (!dir.ok()) return std::printf("statut refus sortie\n"), 0;
    auto file = dir.value().create("tour.ful1");
    if (!file.ok() || !tower::export_full(source, *file.value()).ok() || !dir.value().commit("{}").ok())
      return std::printf("statut refus export\n"), 0;
  }
  std::printf("statut ok\n");
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  u32 threads = 2, slice = 1u << 16;
  std::string dumps;
  for (int i = 1; i < argc; ++i) {
    const std::string opt = argv[i];
    if (opt == "--fils" && i + 1 < argc) threads = static_cast<u32>(std::max(1, std::atoi(argv[++i])));
    else if (opt == "--tranche" && i + 1 < argc) slice = static_cast<u32>(std::max(1, std::atoi(argv[++i])));
    else if (opt == "--vidages" && i + 1 < argc) dumps = argv[++i];
    else return std::fprintf(stderr, "usage : mhgp12_tower_forest_oracle [--fils N] [--tranche E] [--vidages D]\n"), 2;
  }
  auto pool = sched::make_pool(sched::PoolParams{threads});
  if (!pool.ok()) return 2;
  std::ios::sync_with_stdio(false);
  for (u64 number = 0;; ++number) {
    Case c;
    bool done = false;
    if (!read_case(std::cin, c, done)) return std::fprintf(stderr, "entree illisible au cas %llu\n",
                                                           (unsigned long long)number), 2;
    if (done) break;
    play(c, *pool.value(), slice, dumps, number);
  }
  std::printf("fin\n");
  return 0;
}
