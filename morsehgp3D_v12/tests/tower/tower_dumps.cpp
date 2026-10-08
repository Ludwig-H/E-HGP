// mhgp12_tower_dumps : adaptateur de test des vidages MHGP12DP de la v11 gelee (contrat de la tour, paragraphe 2 ;
// format : microbancs/mes_m3_m4_tour/common/format.hpp, lu par son lecteur strict) vers les etages T, M, V et R, puis
// export FUL1 (empreinte, et fichier sur demande). Hors produit : la tour n'a qu'un type d'entree (ForestInput), que
// l'etage G produira ; ici les cibles viennent des vidages.
//
//   mhgp12_tower_dumps <nuage.u32le> <nuage.ids.u32le> <dossier des vidages> [--cibles graines|v12] [--fils N]
//                      [--tranche E] [--repetitions R] [--sortie <dossier>] [--controle-foret]
//
// --cibles graines (defaut) : chaque representant vise la naissance terminale de la descente de la v11 (SEEDS) ;
// --cibles v12 : regle d'arret de la v12 derivee des parties de descente videes (PARTINF) : la premiere partie dont la
//   plus petite boule est une naissance (cible << naissance >>) ou une jonction de l'ordre (cible << cellule >>) ; sinon
//   la graine. Les cellules inertes ne sont pas dans les vidages de la v11 : la descente les traverse (pas valide).
// --controle-foret : compare le registre noeud par noeud a la foret publiee par la v11 (FNODES, FEDGES, FLOWER).
// Sortie : une ligne JSON. Codes : 0 conforme ; 1 ecart au controle de la foret ; 2 usage ; 3 refus ou invariant.
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../../microbancs/mes_m3_m4_tour/common/format.hpp"
#include "io/io.hpp"
#include "sched/sched.hpp"
#include "tower/tower.hpp"

namespace {

using namespace mhgp12;
namespace d = ::mhgp12::dump;

struct Args {
  std::string xyz, ids, dumps, out, mode = "graines";
  u32 threads = 3, slice = 1u << 16, repetitions = 1;
  bool check_forest = false;
};

struct CatalogueData {
  std::vector<std::array<u32, 4>> supports;
  std::vector<num::Level> levels;
  u32 kmax = 0;
};

std::array<u32, 4> support_of(const void* context, u32 ball) noexcept {
  return static_cast<const CatalogueData*>(context)->supports[ball];
}

struct OrderData {
  Order k = 0;
  std::vector<u32> birth_key, targets;
  std::vector<LevelRank> birth_rank, cell_rank;
  std::vector<BallIdx> cell_ball;
  std::vector<u64> rep_offsets;
  tower::ForestInput view() const {
    return tower::ForestInput{k, birth_key, birth_rank, cell_ball, cell_rank, rep_offsets, targets};
  }
};

int refuse(const std::string& why) {
  std::printf("{\"outil\":\"mhgp12_tower_dumps\",\"refus\":\"%s\"}\n", why.c_str());
  return 3;
}

// Catalogue : supports S* par boule, niveaux par rang (forme non reduite de la premiere boule du rang).
bool read_catalogue(const Args& args, const Cloud& cloud, CatalogueData& cat, std::string& why) {
  const d::Reader reader(args.dumps + "/cat.bin");
  const d::Header& h = reader.header();
  if (h.kind != d::kCatalogue || h.kmax < 1 || h.kmax > tower::kMaxOrder || h.sites != cloud.sites())
    return why = "cat.bin : genre, K ou nombre de sites", false;
  cat.kmax = h.kmax;
  const auto xyz = reader.get<u32>("SITEXYZ", 12);
  for (u32 s = 0; s < cloud.sites(); ++s)
    if (xyz.first[3 * s] != cloud.x()[s] || xyz.first[3 * s + 1] != cloud.y()[s] || xyz.first[3 * s + 2] != cloud.z()[s])
      return why = "cat.bin : sites differents du nuage (ordre de Morton)", false;
  const auto balls = reader.get<d::BallRec>("BALLS");
  const auto nlevels = reader.get<u64>("NLEVELS");
  if (nlevels.second != 1 || nlevels.first[0] < 1) return why = "cat.bin : NLEVELS", false;
  cat.supports.resize(balls.second);
  cat.levels.assign(nlevels.first[0], num::Level{});
  u32 last = 0;
  for (u64 b = 0; b < balls.second; ++b) {
    const d::BallRec& rec = balls.first[b];
    for (u32 j = 0; j < 4; ++j) cat.supports[b][j] = rec.sstar[j];
    if (rec.rank == last) continue;
    if (rec.rank != last + 1 || rec.rank >= cat.levels.size()) return why = "cat.bin : rangs non denses", false;
    last = rec.rank;
    std::array<num::Point, 4> p{};
    for (u32 j = 0; j < rec.q; ++j) {
      auto point = num::Point::make(cloud.x()[rec.sstar[j]], cloud.y()[rec.sstar[j]], cloud.z()[rec.sstar[j]]);
      if (!point.ok()) return why = "cat.bin : site hors domaine", false;
      p[j] = point.value();
    }
    auto sphere = rec.q == 2 ? num::Sphere::through(p[0], p[1])
                  : rec.q == 3 ? num::Sphere::through(p[0], p[1], p[2])
                               : num::Sphere::through(p[0], p[1], p[2], p[3]);
    if (!sphere.ok() || !sphere.value()) return why = "cat.bin : support degenere", false;
    cat.levels[rec.rank] = sphere.value()->level();
  }
  if (last + 1 != cat.levels.size()) return why = "cat.bin : niveaux sans boule", false;
  return true;
}

// Cible derivee de la regle de la v12 pour la trace t (mode v12), sinon la graine.
u32 derived_target(const OrderData& o, const d::SeedRec& seed, const d::PartRec* info, u64 first, u64 last,
                   bool v12) {
  if (v12)
    for (u64 p = first; p < last; ++p) {
      const u32 ball = info[p].ball;
      if (ball == d::kNone) continue;
      const auto b = std::lower_bound(o.birth_key.begin(), o.birth_key.end(), ball);
      if (o.k >= 2 && b != o.birth_key.end() && *b == ball) return static_cast<u32>(b - o.birth_key.begin());
      const auto c = std::lower_bound(o.cell_ball.begin(), o.cell_ball.end(), make_id<BallIdx>(ball));
      if (c != o.cell_ball.end() && idx(*c) == ball) return cell_target(static_cast<u32>(c - o.cell_ball.begin()));
    }
  const auto b = std::lower_bound(o.birth_key.begin(), o.birth_key.end(), seed.key);
  return b != o.birth_key.end() && *b == seed.key ? static_cast<u32>(b - o.birth_key.begin()) : kNoTarget;
}

bool read_order(const Args& args, u32 k, OrderData& o, std::string& why) {
  const d::Reader reader(args.dumps + "/ordre_" + std::to_string(k) + ".bin");
  if (reader.header().kind != d::kOrder || reader.header().order != k) return why = "ordre_k.bin : genre ou ordre", false;
  const auto births = reader.get<d::BirthRec>("BIRTHS");
  const auto cells = reader.get<d::CellRec>("CELLS");
  const auto offsets = reader.get<u64>("CELLOFF");
  const auto seeds = reader.get<d::SeedRec>("SEEDS");
  const auto partoff = reader.get<u64>("PARTOFF");
  const auto info = reader.get<d::PartRec>("PARTINF");
  if (offsets.second != cells.second + 1 || offsets.first[cells.second] != seeds.second ||
      partoff.second != seeds.second + 1 || partoff.first[seeds.second] != info.second)
    return why = "ordre_k.bin : sections incoherentes", false;
  o.k = static_cast<Order>(k);
  for (u64 i = 0; i < births.second; ++i) {
    o.birth_key.push_back(births.first[i].key);
    o.birth_rank.push_back(make_id<LevelRank>(births.first[i].rank));
  }
  for (u64 t = 0; t < cells.second; ++t) {
    o.cell_ball.push_back(make_id<BallIdx>(cells.first[t].ball));
    o.cell_rank.push_back(make_id<LevelRank>(cells.first[t].rank));
  }
  o.rep_offsets.assign(offsets.first, offsets.first + offsets.second);
  const bool v12 = args.mode == "v12";
  for (u64 t = 0; t < seeds.second; ++t)
    o.targets.push_back(derived_target(o, seeds.first[t], info.first, partoff.first[t], partoff.first[t + 1], v12));
  return true;
}


bool parse_args(int argc, char** argv, Args& a) {
  if (argc < 4) return false;
  a.xyz = argv[1];
  a.ids = argv[2];
  a.dumps = argv[3];
  for (int i = 4; i < argc; ++i) {
    const std::string opt = argv[i];
    const bool more = i + 1 < argc;
    if (opt == "--cibles" && more) a.mode = argv[++i];
    else if (opt == "--fils" && more) a.threads = static_cast<u32>(std::max(1, std::atoi(argv[++i])));
    else if (opt == "--tranche" && more) a.slice = static_cast<u32>(std::max(1, std::atoi(argv[++i])));
    else if (opt == "--repetitions" && more) a.repetitions = static_cast<u32>(std::max(1, std::atoi(argv[++i])));
    else if (opt == "--sortie" && more) a.out = argv[++i];
    else if (opt == "--controle-foret") a.check_forest = true;
    else return false;
  }
  return a.mode == "graines" || a.mode == "v12";
}

// Registre contre la foret publiee par la v11 : rend le nombre d'ecarts (noeuds, enfants, verticales, racine).
u64 compare_forest(const Args& args, const tower::OrderForest& f, std::string& first) {
  const d::Reader reader(args.dumps + "/foret_" + std::to_string(f.k) + ".bin");
  const auto nodes = reader.get<d::NodeRec>("FNODES");
  const auto edges = reader.get<u32>("FEDGES");
  const auto meta = reader.get<u64>("FMETA");
  u64 gaps = 0;
  auto gap = [&](const std::string& what) {
    if (gaps++ == 0) first = "ordre " + std::to_string(f.k) + " : " + what;
  };
  if (nodes.second != f.nodes() || meta.second != 3 || meta.first[0] != f.births || meta.first[1] != f.root)
    gap("forme (noeuds, naissances, racine)");
  for (u32 v = 0; v < std::min<u64>(nodes.second, f.nodes()); ++v) {
    const d::NodeRec& n = nodes.first[v];
    const u32 key = v < f.births ? f.birth_key[v] : kNone;
    const u64 begin = v < f.births ? 0 : f.children.off[v], count = f.children.off[u64{v} + 1] - f.children.off[v];
    if (n.rank != f.rank[v] || n.parent != f.parent[v] || n.birth_key != key || n.child_count != count ||
        n.child_begin != begin)
      gap("noeud " + std::to_string(v));
  }
  if (edges.second != f.children.val.size()) gap("nombre d'aretes");
  else
    for (u64 j = 0; j < edges.second; ++j)
      if (edges.first[j] != f.children.val[j]) gap("arete " + std::to_string(j));
  if (f.k >= 2) {
    const auto lower = reader.get<u32>("FLOWER");
    if (lower.second != f.nodes()) gap("nombre de verticales");
    else
      for (u32 v = 0; v < f.nodes(); ++v)
        if (lower.first[v] != f.lower[v]) gap("verticale du noeud " + std::to_string(v));
  }
  return gaps;
}

void print_order(const tower::ForestLedger& l, u32 i, const tower::ForestPhysical& best, bool comma) {
  const tower::ForestObject& o = l.object[i];
  const tower::ForestWork& w = l.work[i];
  std::printf("%s{\"k\":%u,\"naissances\":%llu,\"fusions\":%llu,\"verticales\":%llu,\"cellules\":%llu,"
              "\"inertes\":%llu,\"representants\":%llu,\"arite_max\":%llu,\"arites\":[",
              comma ? "," : "", i + 1, (unsigned long long)o.births, (unsigned long long)o.merges,
              (unsigned long long)o.verticals, (unsigned long long)o.cells, (unsigned long long)o.inert_cells,
              (unsigned long long)o.representatives, (unsigned long long)o.max_arity);
  for (u32 a = 0; a < o.arity.size(); ++a) std::printf("%s%llu", a ? "," : "", (unsigned long long)o.arity[a]);
  std::printf("],\"travail\":{\"cohortes\":%llu,\"cohorte_max\":%llu,\"cibles_naissance\":%llu,"
              "\"cibles_cellule\":%llu,\"evenements\":%llu,\"profondeur_attache\":%llu,\"classes\":%llu,"
              "\"cellules_retenues\":%llu,\"t6_cellule\":%llu,\"t6_remontees\":%llu,\"t6_naissance\":%llu,"
              "\"t5_requetes\":%llu,\"t5_sauts\":%llu,\"t5_sauts_max\":%llu,\"t5_sondes\":%llu,"
              "\"branches_relues\":%llu,\"branches\":%llu},",
              (unsigned long long)w.cohorts, (unsigned long long)w.max_cohort, (unsigned long long)w.birth_targets,
              (unsigned long long)w.cell_targets, (unsigned long long)w.events,
              (unsigned long long)w.max_attach_depth, (unsigned long long)w.classes,
              (unsigned long long)w.retained_cells, (unsigned long long)w.t6_from_cell, (unsigned long long)w.t6_climbs,
              (unsigned long long)w.t6_from_birth, (unsigned long long)w.t5_queries, (unsigned long long)w.t5_hops,
              (unsigned long long)w.t5_max_hops, (unsigned long long)w.t5_probes, (unsigned long long)w.branch_reads,
              (unsigned long long)w.branches);
  std::printf("\"ns_min\":{\"naissances\":%llu,\"feuilles\":%llu,\"noyau\":%llu,\"historique\":%llu,"
              "\"contraction\":%llu,\"verticales\":%llu,\"registre\":%llu}}",
              (unsigned long long)best.births_ns, (unsigned long long)best.leaves_ns, (unsigned long long)best.kernel_ns,
              (unsigned long long)best.history_ns, (unsigned long long)best.contraction_ns,
              (unsigned long long)best.vertical_ns, (unsigned long long)best.registry_ns);
}

// Minimum par champ des durees de plusieurs repetitions.
void keep_best(tower::ForestLedger& best, const tower::ForestLedger& now, bool first) {
  auto low = [first](u64& a, u64 b) { a = first ? b : std::min(a, b); };
  for (u32 i = 0; i < tower::kMaxOrder; ++i) {
    low(best.physical[i].births_ns, now.physical[i].births_ns);
    low(best.physical[i].leaves_ns, now.physical[i].leaves_ns);
    low(best.physical[i].kernel_ns, now.physical[i].kernel_ns);
    low(best.physical[i].history_ns, now.physical[i].history_ns);
    low(best.physical[i].contraction_ns, now.physical[i].contraction_ns);
    low(best.physical[i].vertical_ns, now.physical[i].vertical_ns);
    low(best.physical[i].registry_ns, now.physical[i].registry_ns);
  }
  low(best.kernels_ns, now.kernels_ns);
  low(best.contraction_ns, now.contraction_ns);
  low(best.vertical_births_ns, now.vertical_births_ns);
  low(best.vertical_merges_ns, now.vertical_merges_ns);
  low(best.registry_ns, now.registry_ns);
}

int write_file(const Args& args, const tower::FullSource& source) {
  auto dir = io::OutputDirectory::plan(args.out.c_str(), {});
  if (!dir.ok()) return refuse("sortie : " + std::string(reason_name(dir.outcome().reason)));
  auto file = dir.value().create("tour.ful1");
  if (!file.ok()) return refuse("sortie : " + std::string(reason_name(file.outcome().reason)));
  const Outcome written = tower::export_full(source, *file.value());
  if (!written.ok()) return refuse("export : " + std::string(reason_name(written.reason)));
  const Outcome done = dir.value().commit("{\"fichier\":\"tour.ful1\"}");
  if (!done.ok()) return refuse("commit : " + std::string(reason_name(done.reason)));
  return 0;
}

int run_tool(const Args& args) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto input = io::read_u32le(args.xyz.c_str(), args.ids.c_str(), budget);
  if (!input.ok()) return refuse("entree : " + std::string(reason_name(input.outcome().reason)));
  auto cloud = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                             input.value().ids.span(), CoordWidth(), budget);
  if (!cloud.ok()) return refuse("nuage : " + std::string(reason_name(cloud.outcome().reason)));
  CatalogueData cat;
  std::string why;
  if (!read_catalogue(args, cloud.value(), cat, why)) return refuse(why);
  std::vector<OrderData> orders(cat.kmax);
  for (u32 k = 1; k <= cat.kmax; ++k)
    if (!read_order(args, k, orders[k - 1], why)) return refuse(why);
  std::vector<tower::ForestInput> inputs;
  for (const OrderData& o : orders) inputs.push_back(o.view());
  auto pool = sched::make_pool(sched::PoolParams{args.threads});
  if (!pool.ok()) return refuse("pool");
  const tower::BallSource balls{&cat, cat.supports.size(), support_of};
  tower::ForestParams params;
  params.slice_events = args.slice;
  tower::ForestLedger ledger, best;
  Result<tower::TowerForests> forests = fail(Reason::tower_invariant);
  u64 wall = 0;
  for (u32 r = 0; r < args.repetitions; ++r) {
    const auto t0 = std::chrono::steady_clock::now();
    forests = tower::build_forests(cloud.value(), balls, inputs, params, budget, *pool.value(), &ledger);
    const u64 ns = static_cast<u64>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - t0).count());
    if (!forests.ok()) return refuse("tour : " + std::string(reason_name(forests.outcome().reason)));
    wall = r == 0 ? ns : std::min(wall, ns);
    keep_best(best, ledger, r == 0);
  }
  const Outcome valid = tower::validate_forests(forests.value(), budget);
  if (!valid.ok()) return refuse("validation : " + std::string(reason_name(valid.reason)));
  const tower::FullSource source{&cloud.value(), cat.levels, balls, &forests.value()};
  u64 bytes = 0;
  auto digest = tower::full_digest(source, &bytes);
  if (!digest.ok()) return refuse("empreinte : " + std::string(reason_name(digest.outcome().reason)));
  if (!args.out.empty() && write_file(args, source) != 0) return 3;
  u64 gaps = 0;
  std::string first;
  if (args.check_forest)
    for (u32 i = 0; i < cat.kmax; ++i) gaps += compare_forest(args, forests.value().orders[i], first);
  const auto hex = io::to_hex(digest.value());
  std::printf("{\"outil\":\"mhgp12_tower_dumps\",\"profil\":%d,\"K\":%u,\"cibles\":\"%s\",\"fils\":%u,"
              "\"tranches\":%llu,\"repetitions\":%u,\"sha256\":\"%.*s\",\"octets\":%llu,\"validation\":\"ok\","
              "\"controle_foret\":%s,\"ecarts_foret\":%llu,\"premier_ecart\":\"%s\",\"mur_min_ns\":%llu,"
              "\"etages_min_ns\":{\"noyaux\":%llu,\"contraction\":%llu,\"verticales_naissances\":%llu,"
              "\"verticales_fusions\":%llu,\"registre\":%llu},\"ordres\":[",
              kCoordBits, cat.kmax, args.mode.c_str(), args.threads, (unsigned long long)ledger.slices,
              args.repetitions, static_cast<int>(hex.size()), hex.data(), (unsigned long long)bytes,
              args.check_forest ? "true" : "false", (unsigned long long)gaps, first.c_str(),
              (unsigned long long)wall, (unsigned long long)best.kernels_ns, (unsigned long long)best.contraction_ns,
              (unsigned long long)best.vertical_births_ns, (unsigned long long)best.vertical_merges_ns,
              (unsigned long long)best.registry_ns);
  for (u32 i = 0; i < cat.kmax; ++i) print_order(ledger, i, best.physical[i], i > 0);
  std::printf("]}\n");
  return gaps == 0 ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv) {
  Args args;
  if (!parse_args(argc, argv, args)) {
    std::fprintf(stderr, "usage : mhgp12_tower_dumps <xyz> <ids> <vidages> [--cibles graines|v12] [--fils N] "
                         "[--tranche E] [--repetitions R] [--sortie D] [--controle-foret]\n");
    return 2;
  }
  try {
    return run_tool(args);
  } catch (const std::exception& e) {
    return refuse(std::string("exception : ") + e.what());
  }
}
