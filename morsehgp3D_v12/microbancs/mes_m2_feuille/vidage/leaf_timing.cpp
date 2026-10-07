// Chrono local indicatif a un fil (MES-M2, hors produit) : sur les feuilles d'un vidage MHGP12LF, joue en alternance
//   leafcpp   : leaf.cpp de la v11 (enumerate_leaf, voie graphe de paires ; materialise le Level de chaque emission) ;
//   v11dev    : feuille source unique leaf_device.hpp de la v11 sur l'hote (puits de comptage, sans Level) ;
//   j3, coherent : les deux formes v12, warp simule sur l'hote (puits de comptage).
// Repetitions entrelacees (ordre tournant), minimum et mediane par variante. Aucun temps local ne predit G4.
//
// Usage : mhgp12_leaf_timing <vidage.bin> [repetitions=3] [feuilles=toutes]
#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "catalogue/internal.hpp"
#include "catalogue/leaf_device.hpp"
#include "mhgp12/leaf/dump_format.hpp"
#include "mhgp12/leaf/leaf_coherent.hpp"
#include "mhgp12/leaf/leaf_j3.hpp"

using namespace mhgp11;
using namespace mhgp11::catalogue_detail;
namespace dump = mhgp12::dump;
namespace leaf = mhgp12::leaf;

namespace {

u64 now_ns() {
  return static_cast<u64>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                              std::chrono::steady_clock::now().time_since_epoch())
                              .count());
}

struct CountingSink {
  u64 balls = 0, incidences = 0;
  void emit(const leaf::Emission& e) {
    ++balls;
    incidences += u64(e.p) + e.m;
  }
};

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2 || argc > 4) return 2;
  dump::LeafDump d;
  std::string error;
  if (!dump::read(argv[1], d, error)) {
    std::cerr << error << '\n';
    return 2;
  }
  const int reps = argc >= 3 ? std::stoi(argv[2]) : 3;
  const u64 n = argc >= 4 ? std::min<u64>(std::stoull(argv[3]), d.header.n_leaves) : d.header.n_leaves;
  // Cloud v11 reconstruit depuis le vidage (memes SiteIdx : coordonnees deja dans l'ordre du Cloud prepare).
  MemoryBudget budget(u64{16} << 30);
  std::vector<PointId> ids(d.header.n_sites);
  for (u64 s = 0; s < d.header.n_sites; ++s) ids[s] = make_id<PointId>(static_cast<u32>(s));
  auto cloud = prepare_cloud(d.x, d.y, d.z, ids, CoordWidth(), budget);
  if (!cloud.ok()) return 2;
  const Cloud& c = cloud.value();
  for (u64 s = 0; s < d.header.n_sites; ++s)
    if (c.x()[s] != d.x[s] || c.y()[s] != d.y[s] || c.z()[s] != d.z[s]) {
      std::cerr << "ordre des sites different apres prepare_cloud\n";
      return 2;
    }
  CatalogueParams params;
  params.kmax = static_cast<int>(d.header.kmax);
  params.leaf_size = static_cast<u32>(d.header.leaf_size);
  params.max_leaf = 256;
  params.cache_center_lines = (d.header.flags & dump::kFlagCache) != 0;
  params.pair_graph = true;
  Workspace space;
  if (!space.allocate(dump::kMaxLeafSites, budget, params.cache_center_lines, params.pair_graph).ok()) return 2;
  Collector collector;  // comptage seulement
  Run run{c, params, budget, space, collector, {}, nullptr, nullptr};
  std::vector<SiteIdx> local(dump::kMaxLeafSites);
  auto shared = std::make_unique<leaf::j3::SharedJ3>();
  const char* names[4] = {"leafcpp", "v11dev", "j3", "coherent"};
  std::vector<std::vector<double>> seconds(4);
  u64 balls[4] = {0, 0, 0, 0};
  for (int r = 0; r < reps; ++r)
    for (int t = 0; t < 4; ++t) {
      const int v = (t + r) % 4;
      u64 total = 0;
      const u64 start = now_ns();
      for (u64 j = 0; j < n; ++j) {
        const auto& job = d.jobs[j];
        if (v == 0) {
          for (u32 i = 0; i < job.m; ++i) local[i] = make_id<SiteIdx>(d.leaf_sites(j)[i]);
          Box box;
          for (int a = 0; a < 3; ++a) {
            box.lo[a] = job.lo[a];
            box.hi[a] = job.hi[a];
          }
          if (!enumerate_leaf(run, std::span<const SiteIdx>(local.data(), job.m), box).ok()) return 3;
        } else if (v == 1) {
          leaf_device::Input in;
          in.x = d.x.data();
          in.y = d.y.data();
          in.z = d.z.data();
          in.sites = d.leaf_sites(j);
          in.m = job.m;
          for (int a = 0; a < 3; ++a) {
            in.lo[a] = job.lo[a];
            in.hi[a] = job.hi[a];
          }
          in.kmax = params.kmax;
          in.cache = params.cache_center_lines;
          leaf_device::Counts counts;
          leaf_device::CountSink sink;
          if (leaf_device::run_leaf(in, counts, sink) != leaf_device::kOk) return 3;
          total += sink.balls;
        } else {
          leaf::Input in;
          in.x = d.x.data();
          in.y = d.y.data();
          in.z = d.z.data();
          in.sites = d.leaf_sites(j);
          in.m = job.m;
          for (int a = 0; a < 3; ++a) {
            in.lo[a] = job.lo[a];
            in.hi[a] = job.hi[a];
          }
          in.kmax = params.kmax;
          in.cache = params.cache_center_lines;
          leaf::Counts counts;
          CountingSink sink;
          const u32 status = v == 2 ? leaf::j3::run_leaf(in, *shared, counts, sink)
                                    : leaf::coherent::run_leaf(in, *shared, counts, sink);
          if (status != leaf::kStatusOk) return 3;
          total += sink.balls;
        }
      }
      seconds[v].push_back(double(now_ns() - start) * 1e-9);
      balls[v] = v == 0 ? collector.balls : total;
      collector.balls = collector.incidences = 0;
    }
  std::cout << "{\"dump\":\"" << argv[1] << "\",\"leaves\":" << n << ",\"reps\":" << reps;
  for (int v = 0; v < 4; ++v) {
    auto s = seconds[v];
    std::sort(s.begin(), s.end());
    std::cout << ",\"" << names[v] << "\":{\"min_s\":" << s.front() << ",\"median_s\":" << s[s.size() / 2]
              << ",\"balls\":" << balls[v] << '}';
  }
  std::cout << "}\n";
  return 0;
}
