// mhgp12_tower_chain : la chaine de la tour, voie CPU de reference, du nuage au vidage FUL1 : entree -> nuage -> index
// -> catalogue de T1 -> resolution (etage G, resolve_tower) -> T, M, V, R (build_forests par l'adaptateur
// tower::forest_input de chaque ResolvedOrder, supports et niveaux lus dans le catalogue) -> validation -> export.
// Juge la jonction G -> T des la fusion des deux parties du module (MES-M0 semantique, tests/tower/mes_m0.py, mode
// --chaine). Construit seulement si les sources de l'etage G sont presentes (tests/tower/tests.cmake).
//
//   mhgp12_tower_chain <nuage.u32le> <nuage.ids.u32le> <K> [--feuille F] [--fils N] [--tranche E] [--sortie <dossier>]
//
// Sortie : une ligne JSON (empreinte SHA-256 des octets, longueur, compteurs par ordre, durees par etage). Codes : 0
// conforme ; 2 usage ; 3 refus (raison et ordre dans la ligne JSON).
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "catalogue/catalogue.hpp"
#include "index/index.hpp"
#include "io/io.hpp"
#include "sched/sched.hpp"
#include "tower/tower.hpp"

namespace {

using namespace mhgp12;
using Clock = std::chrono::steady_clock;

struct Args {
  std::string xyz, ids, out;
  int kmax = 5;
  u32 leaf = 24, threads = 3, slice = 1u << 16;
};

int refuse(const char* stage, const Outcome& o) {
  std::printf("{\"outil\":\"mhgp12_tower_chain\",\"refus\":\"%s\",\"raison\":\"%s\",\"ordre\":%u}\n", stage,
              std::string(reason_name(o.reason)).c_str(), static_cast<unsigned>(o.order));
  return 3;
}

u64 since(Clock::time_point t0) {
  return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(Clock::now() - t0).count());
}

bool parse(int argc, char** argv, Args& a) {
  if (argc < 4) return false;
  a.xyz = argv[1];
  a.ids = argv[2];
  a.kmax = std::atoi(argv[3]);
  for (int i = 4; i < argc; ++i) {
    const std::string opt = argv[i];
    const bool more = i + 1 < argc;
    if (opt == "--feuille" && more) a.leaf = static_cast<u32>(std::max(1, std::atoi(argv[++i])));
    else if (opt == "--fils" && more) a.threads = static_cast<u32>(std::max(1, std::atoi(argv[++i])));
    else if (opt == "--tranche" && more) a.slice = static_cast<u32>(std::max(1, std::atoi(argv[++i])));
    else if (opt == "--sortie" && more) a.out = argv[++i];
    else return false;
  }
  return a.kmax >= 1 && a.kmax <= 12;
}

int write_dump(const std::string& out, const tower::FullSource& source) {
  auto dir = io::OutputDirectory::plan(out.c_str(), {});
  if (!dir.ok()) return refuse("sortie", dir.outcome());
  auto file = dir.value().create("tour.ful1");
  if (!file.ok()) return refuse("sortie", file.outcome());
  const Outcome written = tower::export_full(source, *file.value());
  if (!written.ok()) return refuse("export", written);
  const Outcome done = dir.value().commit("{\"fichier\":\"tour.ful1\"}");
  return done.ok() ? 0 : refuse("commit", done);
}

void print(const Args& a, const io::Digest& digest, u64 bytes, const tower::ForestLedger& l,
           const std::array<u64, 5>& ns) {
  const auto hex = io::to_hex(digest);
  std::printf("{\"outil\":\"mhgp12_tower_chain\",\"profil\":%d,\"K\":%d,\"fils\":%u,\"sha256\":\"%.*s\","
              "\"octets\":%llu,\"validation\":\"ok\",\"etages_ns\":{\"index\":%llu,\"catalogue\":%llu,"
              "\"resolution\":%llu,\"foret\":%llu,\"empreinte\":%llu},\"ordres\":[",
              kCoordBits, a.kmax, a.threads, static_cast<int>(hex.size()), hex.data(), (unsigned long long)bytes,
              (unsigned long long)ns[0], (unsigned long long)ns[1], (unsigned long long)ns[2],
              (unsigned long long)ns[3], (unsigned long long)ns[4]);
  for (u32 i = 0; i < l.kmax; ++i) {
    const tower::ForestObject& o = l.object[i];
    const tower::ForestWork& w = l.work[i];
    std::printf("%s{\"k\":%u,\"naissances\":%llu,\"fusions\":%llu,\"cellules\":%llu,\"inertes\":%llu,"
                "\"representants\":%llu,\"cibles_cellule\":%llu,\"evenements\":%llu,\"t6_remontees\":%llu,"
                "\"t5_requetes\":%llu}",
                i ? "," : "", i + 1, (unsigned long long)o.births, (unsigned long long)o.merges,
                (unsigned long long)o.cells, (unsigned long long)o.inert_cells, (unsigned long long)o.representatives,
                (unsigned long long)w.cell_targets, (unsigned long long)w.events, (unsigned long long)w.t6_climbs,
                (unsigned long long)w.t5_queries);
  }
  std::printf("]}\n");
}

int run(const Args& a) {
  MemoryBudget budget(MemoryBudget::kUnlimited);
  auto pool = sched::make_pool(sched::PoolParams{a.threads});
  if (!pool.ok()) return refuse("pool", pool.outcome());
  std::array<u64, 5> ns{};
  auto t0 = Clock::now();
  auto input = io::read_u32le(a.xyz.c_str(), a.ids.c_str(), budget);
  if (!input.ok()) return refuse("entree", input.outcome());
  auto cloud = prepare_cloud(input.value().x.span(), input.value().y.span(), input.value().z.span(),
                             input.value().ids.span(), CoordWidth(), budget);
  if (!cloud.ok()) return refuse("nuage", cloud.outcome());
  auto index = build_index(std::move(cloud).take(), IndexParams{}, budget);
  if (!index.ok()) return refuse("index", index.outcome());
  ns[0] = since(t0);
  CatalogueParams params;
  params.kmax = a.kmax;
  params.leaf_size = a.leaf;
  t0 = Clock::now();
  auto catalogue = build_catalogue(index.value().cloud(), params, budget, *pool.value());
  if (!catalogue.ok()) return refuse("catalogue", catalogue.outcome());
  ns[1] = since(t0);
  t0 = Clock::now();
  auto resolution = resolve_tower(index.value(), catalogue.value(), budget, *pool.value());
  if (!resolution.ok()) return refuse("resolution", resolution.outcome());
  ns[2] = since(t0);
  std::vector<tower::ForestInput> inputs;
  for (Order k = 1; k <= resolution.value().orders(); ++k)
    inputs.push_back(tower::forest_input(resolution.value().order(k)));
  const tower::BallSource balls = tower::catalogue_balls(catalogue.value());
  tower::ForestParams forest_params;
  forest_params.slice_events = a.slice;
  tower::ForestLedger ledger;
  t0 = Clock::now();
  auto forests = tower::build_forests(index.value().cloud(), balls, inputs, forest_params, budget, *pool.value(),
                                      &ledger);
  if (!forests.ok()) return refuse("foret", forests.outcome());
  ns[3] = since(t0);
  const Outcome valid = tower::validate_forests(forests.value(), budget);
  if (!valid.ok()) return refuse("validation", valid);
  const tower::FullSource source{&index.value().cloud(), catalogue.value().levels(), balls, &forests.value()};
  t0 = Clock::now();
  u64 bytes = 0;
  auto digest = tower::full_digest(source, &bytes);
  if (!digest.ok()) return refuse("empreinte", digest.outcome());
  ns[4] = since(t0);
  if (!a.out.empty() && write_dump(a.out, source) != 0) return 3;
  print(a, digest.value(), bytes, ledger, ns);
  return 0;
}

}  // namespace

int main(int argc, char** argv) {
  Args args;
  if (!parse(argc, argv, args)) {
    std::fprintf(stderr, "usage : mhgp12_tower_chain <xyz> <ids> <K> [--feuille F] [--fils N] [--tranche E] "
                         "[--sortie D]\n");
    return 2;
  }
  return run(args);
}
