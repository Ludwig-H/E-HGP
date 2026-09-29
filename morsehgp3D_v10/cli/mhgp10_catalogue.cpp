// Sonde du generateur : lit un nuage u32le (x y z par point) et produit le catalogue critique.
//
//   mhgp10_catalogue IN.u32le --k=K [--threads=W] [--leaf=M] [--dump=FILE]
// Sortie standard : une ligne JSON (comptes par (q_min, p), grand-livre, temps mural).
// --dump : une ligne par boule, ordre canonique : rang q p u flags | S* (coordonnees) | I | U.
// Codes : 0 conforme, 2 refus.
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#include "catalogue/catalogue.hpp"

using namespace mhgp10;

int main(int argc, char** argv) {
  if (argc < 2) {
    std::fprintf(stderr, "usage: mhgp10_catalogue IN.u32le --k=K [--threads=W] [--leaf=M] [--dump=FILE]\n");
    return 2;
  }
  CatalogueParams params;
  unsigned threads = 0;
  std::string dump;
  for (int i = 2; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--k=", 0) == 0) params.kmax = std::stoi(a.substr(4));
    else if (a.rfind("--threads=", 0) == 0) threads = unsigned(std::stoul(a.substr(10)));
    else if (a.rfind("--leaf=", 0) == 0) params.leaf_size = u32(std::stoul(a.substr(7)));
    else if (a.rfind("--dump=", 0) == 0) dump = a.substr(7);
    else if (a.rfind("--max-leaf=", 0) == 0) params.max_leaf = u32(std::stoul(a.substr(11)));
    else {
      std::fprintf(stderr, "option inconnue %s\n", a.c_str());
      return 2;
    }
  }
  FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  std::vector<u32> raw;
  u32 buf[3];
  while (std::fread(buf, 4, 3, f) == 3) raw.insert(raw.end(), buf, buf + 3);
  std::fclose(f);
  const u32 n = static_cast<u32>(raw.size() / 3);
  std::vector<u32> x(n), y(n), z(n), pid(n);
  for (u32 i = 0; i < n; ++i) {
    x[i] = raw[3 * i];
    y[i] = raw[3 * i + 1];
    z[i] = raw[3 * i + 2];
    pid[i] = i;
  }
  const auto t0 = std::chrono::steady_clock::now();
  auto prepared = prepare_cloud(x, y, z, pid, kCoordinateBits);
  if (!prepared.ok()) {
    std::printf("{\"status\":\"%s\",\"reason\":\"%s\"}\n", std::string(status_name(prepared.outcome().status())).c_str(),
                std::string(reason_name(prepared.outcome().reason)).c_str());
    return 2;
  }
  const Cloud& cloud = prepared.value();
  sched::Pool pool(threads);
  const auto t1 = std::chrono::steady_clock::now();
  auto built = build_catalogue(cloud, params, pool);
  const auto t2 = std::chrono::steady_clock::now();
  if (!built.ok()) {
    std::printf("{\"status\":\"%s\",\"reason\":\"%s\"}\n", std::string(status_name(built.outcome().status())).c_str(),
                std::string(reason_name(built.outcome().reason)).c_str());
    return 2;
  }
  const Catalogue& cat = built.value();
  std::vector<std::vector<u64>> by(5, std::vector<u64>(kMaxCatalogueOrder + 1, 0));
  for (u32 b = 0; b < cat.balls(); ++b)
    if (cat.p[b] <= u32(kMaxCatalogueOrder)) ++by[cat.qmin[b]][cat.p[b]];
  const auto& L = cat.ledger;
  std::printf("{\"status\":\"ok\",\"n\":%u,\"sites\":%u,\"K\":%d,\"threads\":%u,\"balls\":%u,\"levels\":%zu,", n,
              cloud.sites(), params.kmax, pool.size(), cat.balls(), cat.level.size());
  std::printf("\"prepare_s\":%.4f,\"catalogue_s\":%.4f,", std::chrono::duration<double>(t1 - t0).count(),
              std::chrono::duration<double>(t2 - t1).count());
  std::printf("\"catalogue_stages\":{\"t_frontier\":%.4f,\"t_boxes\":%.4f,\"t_order\":%.4f,\"t_assemble\":%.4f,"
              "\"t_collect\":%.4f,\"t_sort\":%.4f,\"t_bands\":%.4f,\"t_compare\":%.4f,\"t_ranks\":%.4f,\"t_copy\":%.4f,"
              "\"bands\":%llu,\"band_members\":%llu,\"tasks\":%llu,\"max_task_sites\":%llu},\"by_q_p\":{",
              cat.t_frontier, cat.t_boxes, cat.t_order, cat.t_assemble, cat.t_collect, cat.t_sort, cat.t_bands,
              cat.t_compare, cat.t_ranks, cat.t_copy, (unsigned long long)cat.bands, (unsigned long long)cat.band_members,
              (unsigned long long)cat.tasks, (unsigned long long)cat.max_task_sites);
  for (int q = 2; q <= 4; ++q) {
    std::printf("%s\"q%d\":[", q > 2 ? "," : "", q);
    for (int p = 0; p < params.kmax; ++p) std::printf("%s%llu", p ? "," : "", static_cast<unsigned long long>(by[q][p]));
    std::printf("]");
  }
  std::printf("},\"nodes\":%llu,\"leaves\":%llu,\"sum_m\":%llu,\"max_m\":%llu,\"skipped_bbox\":%llu,"
              "\"filter_tests\":%llu,\"leaf_dominance_tests\":%llu,\"pair_tests\":%llu,"
              "\"triple_tests\":%llu,\"line_hits\":%llu,\"quad_tests\":%llu,\"judged\":%llu,\"extended\":%llu,"
              "\"weighted\":%llu,\"max_shell\":%llu,\"stalled_leaves\":%llu}\n",
              (unsigned long long)L.nodes, (unsigned long long)L.leaves, (unsigned long long)L.sum_m,
              (unsigned long long)L.max_m, (unsigned long long)L.skipped_bbox,
              (unsigned long long)L.filter_tests, (unsigned long long)L.leaf_dominance_tests,
              (unsigned long long)L.pair_tests, (unsigned long long)L.triple_tests, (unsigned long long)L.line_hits,
              (unsigned long long)L.quad_tests, (unsigned long long)L.judged, (unsigned long long)L.extended,
              (unsigned long long)L.weighted, (unsigned long long)L.max_shell, (unsigned long long)L.stalled_leaves);
  if (!dump.empty()) {
    FILE* o = std::fopen(dump.c_str(), "w");
    if (!o) return 2;
    auto pt = [&](u32 s) { std::fprintf(o, " %u,%u,%u", cloud.x[s], cloud.y[s], cloud.z[s]); };
    for (u32 b = 0; b < cat.balls(); ++b) {
      std::fprintf(o, "%u %u %u %u %u |", cat.rank[b], cat.qmin[b], cat.p[b], cat.u[b], cat.flags[b]);
      for (u32 s : cat.support[b])
        if (s != kNone) pt(s);
      std::fprintf(o, " |");
      for (u32 s : cat.interior(b)) pt(s);
      std::fprintf(o, " |");
      for (u32 s : cat.shell(b)) pt(s);
      std::fprintf(o, "\n");
    }
    std::fclose(o);
  }
  return 0;
}
