// Temoin de test (hors produit) de la tete : lit des coordonnees u32le (x y z par point, PointId = rang d'entree),
// construit la hierarchie de points demandee et ecrit une etiquette i32 par point d'entree.
//
//   mhgp10_cluster IN.u32le OUT.i32le --source=mreach --k=5 --mcs=20 [--z=1] [--selection=eom|leaf]
//                  [--allow-single] [--threads=0] [--alpha=1|2] [--configs=FILE]
// --alpha : parametre alpha de scikit-learn (mreach = max(coeurs, distance / alpha)). --configs : une tete par ligne
// « mcs z eom|leaf 0|1 », la i-eme ecrite dans OUT.i, toutes sur la meme hierarchie (diagnostic dev : meme tete sur
// la tour et sur l'atteignabilite mutuelle).
// Codes : 0 conforme, 2 refus avant calcul.
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "cloud/site_tree.hpp"
#include "head/head.hpp"
#include "mreach.hpp"

using namespace mhgp10;

int main(int argc, char** argv) {
  if (argc < 3) {
    std::fprintf(stderr, "usage: mhgp10_cluster IN.u32le OUT.i32le [options]\n");
    return 2;
  }
  std::string source = "mreach";
  u64 k = 5;
  ClusterParams params;
  unsigned threads = 0;
  u64 alpha = 1;
  std::string configs;
  for (int i = 3; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--source=", 0) == 0) source = a.substr(9);
    else if (a.rfind("--k=", 0) == 0) k = std::stoull(a.substr(4));
    else if (a.rfind("--mcs=", 0) == 0) params.min_cluster_size = std::stoull(a.substr(6));
    else if (a.rfind("--z=", 0) == 0) params.z = std::stod(a.substr(4));
    else if (a == "--selection=leaf") params.selection = Selection::leaf;
    else if (a == "--selection=eom") params.selection = Selection::eom;
    else if (a == "--allow-single") params.allow_single_cluster = true;
    else if (a.rfind("--threads=", 0) == 0) threads = unsigned(std::stoul(a.substr(10)));
    else if (a.rfind("--alpha=", 0) == 0) alpha = std::stoull(a.substr(8));
    else if (a.rfind("--configs=", 0) == 0) configs = a.substr(10);
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
  auto prepared = prepare_cloud(x, y, z, pid, 21);
  if (!prepared.ok()) {
    std::fprintf(stderr, "refus %s\n", std::string(reason_name(prepared.outcome().reason)).c_str());
    return 2;
  }
  const Cloud& cloud = prepared.value();
  SiteTree tree(cloud);
  sched::Pool pool(threads);
  if (source != "mreach") {
    std::fprintf(stderr, "source inconnue %s\n", source.c_str());
    return 2;
  }
  if (alpha < 1 || alpha > 2) return 2;
  PointDendrogram d = mreach_dendrogram(tree, k, pool, alpha);
  const Outcome v = validate(d);
  if (!v.ok()) {
    std::fprintf(stderr, "dendrogramme invalide %s\n", std::string(reason_name(v.reason)).c_str());
    return 3;
  }
  std::vector<ClusterParams> list;
  if (configs.empty()) {
    list.push_back(params);
  } else {
    FILE* cf = std::fopen(configs.c_str(), "r");
    if (!cf) return 2;
    unsigned long long m;
    double zz;
    char sel[16];
    int single;
    while (std::fscanf(cf, "%llu %lf %15s %d", &m, &zz, sel, &single) == 4) {
      ClusterParams q;
      q.min_cluster_size = m;
      q.z = zz;
      q.selection = std::string(sel) == "leaf" ? Selection::leaf : Selection::eom;
      q.allow_single_cluster = single != 0;
      list.push_back(q);
    }
    std::fclose(cf);
  }
  for (size_t i = 0; i < list.size(); ++i) {
    Clustering cl = cluster(d, list[i]);
    std::vector<i32> out(n, -1);
    for (u32 s = 0; s < cloud.sites(); ++s)
      for (PointId p : cloud.ids.row(s)) out[idx(p)] = cl.label[s];
    const std::string path = configs.empty() ? std::string(argv[2]) : std::string(argv[2]) + "." + std::to_string(i);
    FILE* o = std::fopen(path.c_str(), "wb");
    if (!o) return 2;
    std::fwrite(out.data(), 4, n, o);
    std::fclose(o);
    std::printf("clusters %zu\n", cl.selected.size());
  }
  return 0;
}
