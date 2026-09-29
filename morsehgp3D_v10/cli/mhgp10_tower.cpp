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
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "tower/tower.hpp"

using namespace mhgp10;

namespace {
// Memoire residente du processus (Linux, /proc/self/status) : VmRSS et VmHWM en kio ; 0 si indisponible.
void resident_kib(unsigned long long& rss, unsigned long long& hwm) {
  rss = hwm = 0;
  FILE* f = std::fopen("/proc/self/status", "r");
  if (!f) return;
  char line[256];
  while (std::fgets(line, sizeof line, f)) {
    if (std::strncmp(line, "VmRSS:", 6) == 0) rss = std::strtoull(line + 6, nullptr, 10);
    if (std::strncmp(line, "VmHWM:", 6) == 0) hwm = std::strtoull(line + 6, nullptr, 10);
  }
  std::fclose(f);
}
}  // namespace

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
  unsigned long long rss_cat = 0, hwm_cat = 0, rss_tow = 0, hwm_tow = 0;
  for (int r = 0; r < repeat; ++r) {
    const auto a = clk::now();
    built = build_catalogue(cloud, cp, pool);
    const auto b = clk::now();
    if (!built.ok()) {
      std::printf("{\"status\":\"%s\",\"reason\":\"%s\"}\n", std::string(status_name(built.outcome().status())).c_str(),
                  std::string(reason_name(built.outcome().reason)).c_str());
      return 2;
    }
    resident_kib(rss_cat, hwm_cat);
    tw = build_tower(cloud, tree, built.value(), tp, pool);
    resident_kib(rss_tow, hwm_tow);
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
              "\"tower_s\":%.4f,\"points\":%s,\"rss_kib\":{\"after_catalogue\":%llu,\"peak_after_catalogue\":%llu,"
              "\"after_tower\":%llu,\"peak_after_tower\":%llu},\"passes_catalogue_s\":[",
              n, kmax, pool.size(), cat.balls(), sec(t0, t1), cat_s.back(), tow_s.back(), points ? "true" : "false", rss_cat,
              hwm_cat, rss_tow, hwm_tow);
  for (size_t i = 0; i < cat_s.size(); ++i) std::printf("%s%.4f", i ? "," : "", cat_s[i]);
  std::printf("],\"passes_tower_s\":[");
  for (size_t i = 0; i < tow_s.size(); ++i) std::printf("%s%.4f", i ? "," : "", tow_s[i]);
  std::printf("],\"orders\":[");
  auto counters = [](const char* name, const ResolveCounters& c) {
    std::printf(",\"%s\":{\"resolves\":%llu,\"steps\":%llu,\"seed_hits\":%llu,\"birth_hits\":%llu,\"memo_hits\":%llu,"
                "\"meb\":%llu,\"knn_queries\":%llu,\"knn_jumps\":%llu,\"closed_balls\":%llu,\"lookups\":%llu,"
                "\"local_calls\":%llu,\"reused\":%llu,\"census_cat\":%llu,\"level_exact\":%llu,\"jump_exact\":%llu}",
                name, (unsigned long long)c.resolves, (unsigned long long)c.steps, (unsigned long long)c.seed_hits,
                (unsigned long long)c.birth_hits, (unsigned long long)c.memo_hits, (unsigned long long)c.meb,
                (unsigned long long)c.knn_queries, (unsigned long long)c.knn_jumps, (unsigned long long)c.closed_balls,
                (unsigned long long)c.lookups, (unsigned long long)c.local_calls, (unsigned long long)c.reused,
                (unsigned long long)c.census_cat, (unsigned long long)c.level_exact, (unsigned long long)c.jump_exact);
  };
  ResolveCounters tj, tp2, tv;
  u64 cells = 0, walks = 0;
  double kr = 0, vt = 0;
  for (const OrderForest& o : tower.orders) {
    const OrderStats& st = o.stats;
    std::printf("%s{\"k\":%d,\"nodes\":%zu,\"births\":%llu,\"merges\":%llu,\"joins\":%llu,\"steps\":%llu,\"memo\":%llu,"
                "\"t_kruskal\":%.4f,\"t_vertical\":%.4f,\"local_cells\":%llu,\"walk_steps\":%llu",
                o.k > 1 ? "," : "", o.k, o.rank.size(), (unsigned long long)o.births, (unsigned long long)o.merges,
                (unsigned long long)o.joins, (unsigned long long)o.descent_steps, (unsigned long long)o.memo_hits,
                st.t_kruskal, st.t_vertical, (unsigned long long)st.local_cells, (unsigned long long)st.walk_steps);
    counters("join", st.join);
    counters("point", st.point);
    counters("vertical", st.vertical);
    std::printf("}");
    tj.add(st.join);
    tp2.add(st.point);
    tv.add(st.vertical);
    cells += st.local_cells;
    walks += st.walk_steps;
    kr += st.t_kruskal;
    vt += st.t_vertical;
  }
  const TowerStats& ts = tower.stats;
  std::printf("],\"stages\":{\"t_prepare\":%.4f,\"t_local\":%.4f,\"t_seeds\":%.4f,\"t_resolve\":%.4f,\"t_kruskal\":%.4f,"
              "\"t_points\":%.4f,\"t_vertical\":%.4f,\"sum_order_kruskal\":%.4f,\"sum_order_vertical\":%.4f,"
              "\"local_cells\":%llu,\"walk_steps\":%llu,\"meb_fallbacks\":%llu",
              ts.t_prepare, ts.t_local, ts.t_seeds, ts.t_resolve, ts.t_kruskal, ts.t_points, ts.t_vertical, kr, vt,
              (unsigned long long)cells, (unsigned long long)walks, (unsigned long long)ts.meb_fallbacks);
  counters("join", tj);
  counters("point", tp2);
  counters("vertical", tv);
  std::printf("}}\n");
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
