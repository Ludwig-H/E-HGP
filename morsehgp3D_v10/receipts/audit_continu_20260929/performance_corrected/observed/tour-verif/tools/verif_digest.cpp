// Outil du verificateur adverse (levier tour, 29 septembre 2026), hors depot.
// Empreinte de TOUS les champs publies de chaque OrderForest (y compris birth, CSR des enfants, ball_node), et de
// l'issue en cas d'echec. Ecrit independamment de la sonde du concepteur (autres constantes, autre melange).
//
//   verif_digest IN.u32le --k=K [--cat-threads=C] [--threads=1,2,3] [--repeat=R] [--only-order=K]
//                [--entry=core|cover] [--no-points] [--no-verticals] [--cover-extra=E] [--ball-nodes]
#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "tower/tower.hpp"

using namespace mhgp10;

namespace {

// Melange 2 x 64 bits (constantes de splitmix64 et de wyhash), sensible a l'ordre.
struct H {
  u64 a = 0x6A09E667F3BCC908ull, b = 0xBB67AE8584CAA73Bull;
  static u64 sm(u64 z) {
    z += 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
  }
  void add(u64 v) {
    a = sm(a ^ v) + 0xa0761d6478bd642full;
    b = sm(b + v * 0xe7037ed1a0b428dbull) ^ a;
  }
  template <class V>
  void vec(const V& v) {
    add(0xABCDEF0000000000ull ^ v.size());
    for (const auto& x : v) add(static_cast<u64>(x));
  }
};

std::vector<unsigned> parse_list(const std::string& s) {
  std::vector<unsigned> out;
  size_t pos = 0;
  while (pos < s.size()) {
    const size_t e = s.find(',', pos);
    out.push_back(unsigned(std::stoul(s.substr(pos, e == std::string::npos ? std::string::npos : e - pos))));
    if (e == std::string::npos) break;
    pos = e + 1;
  }
  return out;
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) return 2;
  int kmax = 5, only = 0, repeat = 1, extra = 0;
  unsigned cat_threads = 2;
  std::vector<unsigned> wlist{1};
  bool points = true, verticals = true, ball_nodes = false;
  PointEntry entry = PointEntry::core;
  for (int i = 2; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--k=", 0) == 0) kmax = std::stoi(a.substr(4));
    else if (a.rfind("--cat-threads=", 0) == 0) cat_threads = unsigned(std::stoul(a.substr(14)));
    else if (a.rfind("--threads=", 0) == 0) wlist = parse_list(a.substr(10));
    else if (a.rfind("--repeat=", 0) == 0) repeat = std::stoi(a.substr(9));
    else if (a.rfind("--only-order=", 0) == 0) only = std::stoi(a.substr(13));
    else if (a.rfind("--cover-extra=", 0) == 0) extra = std::stoi(a.substr(14));
    else if (a == "--entry=cover") entry = PointEntry::cover;
    else if (a == "--entry=core") entry = PointEntry::core;
    else if (a == "--no-points") points = false;
    else if (a == "--no-verticals") verticals = false;
    else if (a == "--ball-nodes") ball_nodes = true;
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
  auto prepared = prepare_cloud(x, y, z, pid, kCoordinateBits);
  if (!prepared.ok()) {
    std::printf("{\"status\":\"cloud_refused\"}\n");
    return 2;
  }
  const Cloud& cloud = prepared.value();
  SiteTree tree(cloud);
  Result<Catalogue> cat = fail(Reason::none);
  {
    sched::Pool cpool(cat_threads);
    CatalogueParams cp;
    cp.kmax = kmax + extra;
    cat = build_catalogue(cloud, cp, cpool);
  }
  if (!cat.ok()) {
    std::printf("{\"status\":\"catalogue_%s\",\"reason\":\"%s\"}\n", std::string(status_name(cat.outcome().status())).c_str(),
                std::string(reason_name(cat.outcome().reason)).c_str());
    return 2;
  }
  TowerParams tp;
  tp.kmax = kmax;
  tp.only_order = only;
  tp.entry = entry;
  tp.points = points;
  tp.verticals = verticals;
  tp.cover_extra = extra;
  tp.ball_nodes = ball_nodes;
  for (int r = 0; r < repeat; ++r) {
    for (size_t ii = 0; ii < wlist.size(); ++ii) {
      const unsigned w = wlist[(r % 2 == 0) ? ii : wlist.size() - 1 - ii];
      sched::Pool pool(w);
      auto tw = build_tower(cloud, tree, cat.value(), tp, pool);
      if (!tw.ok()) {
        std::printf("{\"round\":%d,\"threads\":%u,\"n\":%u,\"balls\":%u,\"status\":\"%s\",\"reason\":\"%s\",\"order\":%d}\n", r, w, n,
                    cat.value().balls(), std::string(status_name(tw.outcome().status())).c_str(),
                    std::string(reason_name(tw.outcome().reason)).c_str(), tw.outcome().order);
        std::fflush(stdout);
        continue;
      }
      const Tower& T = tw.value();
      H h;
      std::string counts;
      for (const OrderForest& o : T.orders) {
        h.add(u64(o.k));
        h.vec(o.rank);
        h.vec(o.parent);
        h.vec(o.child_off);
        h.vec(o.child_val);
        h.vec(o.birth);
        h.vec(o.lower);
        h.vec(o.point_node);
        h.vec(o.point_level);
        h.vec(o.point_cat_rank);
        h.vec(o.ball_node);
        h.add(o.births);
        h.add(o.joins);
        h.add(o.merges);
        if (!o.rank.empty()) {
          char b[128];
          std::snprintf(b, sizeof b, "%s[%d,%zu,%llu]", counts.empty() ? "" : ",", o.k, o.rank.size(),
                        (unsigned long long)o.merges);
          counts += b;
        }
      }
      std::printf("{\"round\":%d,\"threads\":%u,\"n\":%u,\"balls\":%u,\"status\":\"ok\",\"orders\":[%s],\"digest\":\"%016llx%016llx\"}\n",
                  r, w, n, cat.value().balls(), counts.c_str(), (unsigned long long)h.a, (unsigned long long)h.b);
      std::fflush(stdout);
    }
  }
  return 0;
}
