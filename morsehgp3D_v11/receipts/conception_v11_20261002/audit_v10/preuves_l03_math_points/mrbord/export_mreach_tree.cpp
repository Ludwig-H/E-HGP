// Audit L03 : exporte la hierarchie de points d'atteignabilite mutuelle (temoin du depot, tests/head/mreach.cpp)
// au format texte de `mhgp10_cluster --tree`, pour mesurer le niveau B (meilleur bloc) de MR_alpha a entree coeur
// ou bord avec le meme evaluateur que la tour.
//   export_mreach_tree IN.u32le TREE.txt --k=K [--alpha=1|2] [--entry=core|border] [--threads=W]
#include <cstdio>
#include <string>
#include <vector>
#include "cloud/site_tree.hpp"
#include "head/head.hpp"
#include "mreach.hpp"
using namespace mhgp10;
int main(int argc, char** argv) {
  if (argc < 3) return 2;
  u64 k = 5, alpha = 1;
  bool border = false;
  unsigned threads = 1;
  for (int i = 3; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--k=", 0) == 0) k = std::stoull(a.substr(4));
    else if (a.rfind("--alpha=", 0) == 0) alpha = std::stoull(a.substr(8));
    else if (a == "--entry=border") border = true;
    else if (a == "--entry=core") border = false;
    else if (a.rfind("--threads=", 0) == 0) threads = unsigned(std::stoul(a.substr(10)));
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
  for (u32 i = 0; i < n; ++i) { x[i] = raw[3 * i]; y[i] = raw[3 * i + 1]; z[i] = raw[3 * i + 2]; pid[i] = i; }
  auto prepared = prepare_cloud(x, y, z, pid, 21);
  if (!prepared.ok()) return 2;
  const Cloud& cloud = prepared.value();
  SiteTree tree(cloud);
  sched::Pool pool(threads);
  PointDendrogram d = mreach_dendrogram(tree, k, pool, alpha, border);
  if (!validate(d).ok()) return 3;
  FILE* t = std::fopen(argv[2], "w");
  if (!t) return 2;
  std::fprintf(t, "levels %zu\n", d.level.size());
  for (double lv : d.level) std::fprintf(t, "%.17g\n", lv);
  std::fprintf(t, "nodes %u\n", d.nodes());
  for (u32 v = 0; v < d.nodes(); ++v)
    std::fprintf(t, "%u %lld\n", d.node_rank[v], d.parent[v] == kNone ? -1LL : (long long)d.parent[v]);
  std::fprintf(t, "points %u\n", n);
  for (u32 s = 0; s < cloud.sites(); ++s)
    for (PointId p : cloud.ids.row(s)) std::fprintf(t, "%u %u %u %u\n", idx(p), d.point_node[s], d.point_rank[s], 1u);
  std::fclose(t);
  return 0;
}
