// Sonde de la tour : nuage u32le -> catalogue -> tour FULL 1..K -> attaches C n X.
//
//   mhgp10_tower IN.u32le --k=K [--threads=W] [--dump=FILE] [--no-points] [--repeat=R]
// --no-points : tour FULL seule (contrat LiDAR), sans attaches C n X. --repeat : R passes chaudes dans le meme
// processus (catalogue + tour), temps publies par passe.
// Sortie standard : une ligne JSON (temps par etage, noeuds, fusions, descentes).
// --dump : par ordre, les noeuds (parent, niveau exact num/den) et les attaches des points.
// Codes : 0 conforme, 2 refus, 3 invariant viole.
#include <chrono>
#include <cstdio>
#include <string>
#include <vector>

#include "tower/tower.hpp"

using namespace mhgp10;

int main(int argc, char** argv) {
  if (argc < 2) return 2;
  int kmax = 5;
  unsigned threads = 0;
  std::string dump;
  bool points = true;
  int repeat = 1;
  for (int i = 2; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--k=", 0) == 0) kmax = std::stoi(a.substr(4));
    else if (a.rfind("--threads=", 0) == 0) threads = unsigned(std::stoul(a.substr(10)));
    else if (a.rfind("--dump=", 0) == 0) dump = a.substr(7);
    else if (a == "--no-points") points = false;
    else if (a.rfind("--repeat=", 0) == 0) repeat = std::stoi(a.substr(9));
    else return 2;
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
  using clk = std::chrono::steady_clock;
  const auto t0 = clk::now();
  auto prepared = prepare_cloud(x, y, z, pid, kCoordinateBits);
  if (!prepared.ok()) return 2;
  const Cloud& cloud = prepared.value();
  sched::Pool pool(threads);
  SiteTree tree(cloud);
  const auto t1 = clk::now();
  CatalogueParams cp;
  cp.kmax = kmax;
  TowerParams tp;
  tp.kmax = kmax;
  tp.points = points;
  std::vector<double> cat_s, tow_s;
  Result<Catalogue> built = fail(Reason::none);
  Result<Tower> tw = fail(Reason::none);
  clk::time_point t2 = t1, t3 = t1;
  for (int r = 0; r < repeat; ++r) {
    const auto a = clk::now();
    built = build_catalogue(cloud, cp, pool);
    const auto b = clk::now();
    if (!built.ok()) {
      std::printf("{\"status\":\"%s\",\"reason\":\"%s\"}\n", std::string(status_name(built.outcome().status())).c_str(),
                  std::string(reason_name(built.outcome().reason)).c_str());
      return 2;
    }
    tw = build_tower(cloud, tree, built.value(), tp, pool);
    const auto c = clk::now();
    if (!tw.ok()) {
      std::printf("{\"status\":\"%s\",\"reason\":\"%s\",\"order\":%d}\n", std::string(status_name(tw.outcome().status())).c_str(),
                  std::string(reason_name(tw.outcome().reason)).c_str(), tw.outcome().order);
      return tw.outcome().status() == Status::invariant_violated ? 3 : 2;
    }
    cat_s.push_back(std::chrono::duration<double>(b - a).count());
    tow_s.push_back(std::chrono::duration<double>(c - b).count());
    t2 = b;
    t3 = c;
  }
  const Catalogue& cat = built.value();
  const Tower& tower = tw.value();
  auto sec = [](auto a, auto b) { return std::chrono::duration<double>(b - a).count(); };
  std::printf("{\"status\":\"ok\",\"n\":%u,\"K\":%d,\"threads\":%u,\"balls\":%u,\"prepare_s\":%.4f,\"catalogue_s\":%.4f,"
              "\"tower_s\":%.4f,\"points\":%s,\"passes_catalogue_s\":[",
              n, kmax, pool.size(), cat.balls(), sec(t0, t1), cat_s.back(), tow_s.back(), points ? "true" : "false");
  for (size_t i = 0; i < cat_s.size(); ++i) std::printf("%s%.4f", i ? "," : "", cat_s[i]);
  std::printf("],\"passes_tower_s\":[");
  for (size_t i = 0; i < tow_s.size(); ++i) std::printf("%s%.4f", i ? "," : "", tow_s[i]);
  std::printf("],\"orders\":[");
  for (const OrderForest& o : tower.orders)
    std::printf("%s{\"k\":%d,\"nodes\":%zu,\"births\":%llu,\"merges\":%llu,\"joins\":%llu,\"steps\":%llu,\"memo\":%llu}",
                o.k > 1 ? "," : "", o.k, o.rank.size(), (unsigned long long)o.births, (unsigned long long)o.merges,
                (unsigned long long)o.joins, (unsigned long long)o.descent_steps, (unsigned long long)o.memo_hits);
  std::printf("]}\n");
  if (!dump.empty()) {
    FILE* o = std::fopen(dump.c_str(), "w");
    if (!o) return 2;
    for (const OrderForest& ord : tower.orders) {
      std::fprintf(o, "order %d %zu %u\n", ord.k, ord.rank.size(), cloud.sites());
      for (u32 v = 0; v < ord.rank.size(); ++v) {
        std::string num = "0", den = "1";
        if (ord.rank[v] > 0) {
          num = arith::to_string(cat.level[ord.rank[v] - 1].num);
          den = arith::to_string(cat.level[ord.rank[v] - 1].den);
        }
        const long long low = ord.lower.empty() ? -1LL : (long long)ord.lower[v];
        std::fprintf(o, "node %u %lld %s %s %lld\n", v, ord.parent[v] == kNone ? -1LL : (long long)ord.parent[v], num.c_str(),
                     den.c_str(), low);
      }
      for (u32 s = 0; s < cloud.sites(); ++s)
        std::fprintf(o, "point %u %u %u %u %llu\n", cloud.x[s], cloud.y[s], cloud.z[s], ord.point_node[s],
                     (unsigned long long)ord.point_level[s]);
    }
    std::fclose(o);
  }
  return 0;
}
