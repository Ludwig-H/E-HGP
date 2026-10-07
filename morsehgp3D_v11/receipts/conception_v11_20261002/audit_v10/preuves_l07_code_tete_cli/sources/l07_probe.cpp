// Audit L07 (2 octobre 2026) : sonde hors produit. Lie libmhgp10_core.a (afb081774) et le temoin mreach.
//
//   l07_probe run IN.u32le OUTPREFIX --k-list=2,3,5 --entries=core,cover --configs=FILE [--threads=T]
//             [--mreach=ALPHA:core|border,...] [--dump=diff|all|none]
//   Par (source, K, configuration) : tete publiee V (mhgp10::cluster), port V' (doit egaler V bit pour bit), tete a
//   cohortes C (meme dendrogramme), tete a cohortes N sur le dendrogramme normalise, et VN (tete publiee sur le
//   dendrogramme normalise). Une ligne JSON par cas.
//
//   l07_probe search --n=N --grid=G --dim=3 --k=K --entry=core|cover --trials=T --seed=S --mcs=2,3 --z=1,2
//   Cherche les plus petits nuages reels dont les etiquettes changent entre V et N.
#include <chrono>
#include <cstdio>
#include <cstring>
#include <random>
#include <set>
#include <string>
#include <vector>

#include "l07_heads.hpp"
#include "mreach.hpp"
#include "tower/tower.hpp"

using namespace mhgp10;
using Clock = std::chrono::steady_clock;

static double secs(Clock::time_point a, Clock::time_point b) { return std::chrono::duration<double>(b - a).count(); }

static std::vector<int> ints(const std::string& v) {
  std::vector<int> out;
  size_t pos = 0;
  while (pos < v.size()) {
    const size_t e = v.find(',', pos);
    out.push_back(std::stoi(v.substr(pos, e == std::string::npos ? std::string::npos : e - pos)));
    if (e == std::string::npos) break;
    pos = e + 1;
  }
  return out;
}
static std::vector<std::string> strs(const std::string& v) {
  std::vector<std::string> out;
  size_t pos = 0;
  while (pos <= v.size()) {
    const size_t e = v.find(',', pos);
    out.push_back(v.substr(pos, e == std::string::npos ? std::string::npos : e - pos));
    if (e == std::string::npos) break;
    pos = e + 1;
  }
  return out;
}

static bool same_bits(const std::vector<double>& a, const std::vector<double>& b) {
  return a.size() == b.size() && (a.empty() || std::memcmp(a.data(), b.data(), a.size() * sizeof(double)) == 0);
}

struct Compare {
  bool port_ok = false;       // V' == V (etiquettes, selection, stabilites bit pour bit)
  u32 clusters = 0, leaves = 0;
  u32 sel_v = 0, sel_c = 0, sel_n = 0, sel_vn = 0;
  u64 lab_vc = 0, lab_vn = 0, lab_cn = 0, lab_vvn = 0;  // sites dont l'etiquette brute differe
  bool selset_vc = true, selset_vn = true;              // memes ensembles retenus (a structure egale)
  u64 stab_changed = 0;                                 // clusters dont la stabilite change de plus de 1e-12 relatif (V contre C)
  double stab_ratio_max = 1.0;                          // max stabilite V / stabilite C
  double leaf_sum_v = 0, leaf_sum_c = 0;                // somme des stabilites des feuilles de l'arbre condense
  l07::Trig trig_c, trig_n;
  l07::Walk walk;
  u64 noise_v = 0, noise_n = 0;
  double t_v = 0, t_c = 0;
  Clustering V, C, N;
};

static u64 count_diff(const std::vector<i32>& a, const std::vector<i32>& b) {
  u64 d = 0;
  for (size_t i = 0; i < a.size(); ++i) d += a[i] != b[i];
  return d;
}

static Compare compare(const PointDendrogram& d, const PointDendrogram& dn, const ClusterParams& p) {
  Compare r;
  auto t0 = Clock::now();
  r.V = cluster(d, p);
  auto t1 = Clock::now();
  Clustering Vp = l07::select_x(l07::condense_x(d, p, false), d.points(), p, &r.walk);
  r.port_ok = Vp.label == r.V.label && Vp.selected == r.V.selected && same_bits(Vp.tree.stability, r.V.tree.stability) &&
              Vp.tree.parent == r.V.tree.parent && Vp.tree.point_cluster == r.V.tree.point_cluster &&
              same_bits(Vp.tree.point_lambda, r.V.tree.point_lambda) && Vp.cluster_label == r.V.cluster_label &&
              Vp.tree.node_cluster == r.V.tree.node_cluster;
  auto t2 = Clock::now();
  r.C = l07::select_x(l07::condense_x(d, p, true, &r.trig_c), d.points(), p);
  auto t3 = Clock::now();
  Clustering VN = l07::select_x(l07::condense_x(dn, p, false), dn.points(), p);
  r.N = l07::select_x(l07::condense_x(dn, p, true, &r.trig_n), dn.points(), p);
  r.t_v = secs(t0, t1);
  r.t_c = secs(t2, t3);
  r.clusters = static_cast<u32>(r.V.tree.parent.size());
  std::vector<unsigned char> has_kid(r.clusters, 0);
  for (u32 c = 1; c < r.clusters; ++c) has_kid[r.V.tree.parent[c]] = 1;
  for (u32 c = 0; c < r.clusters; ++c) r.leaves += !has_kid[c];
  r.sel_v = static_cast<u32>(r.V.selected.size());
  r.sel_c = static_cast<u32>(r.C.selected.size());
  r.sel_n = static_cast<u32>(r.N.selected.size());
  r.sel_vn = static_cast<u32>(VN.selected.size());
  r.lab_vc = count_diff(r.V.label, r.C.label);
  r.lab_vn = count_diff(r.V.label, r.N.label);
  r.lab_cn = count_diff(r.C.label, r.N.label);
  r.lab_vvn = count_diff(r.V.label, VN.label);
  r.selset_vc = r.V.tree.parent == r.C.tree.parent && r.V.selected == r.C.selected;
  r.selset_vn = r.V.tree.parent == r.N.tree.parent && r.V.selected == r.N.selected;
  if (r.V.tree.parent == r.C.tree.parent)
    for (u32 c = 0; c < r.clusters; ++c) {
      const double a = r.V.tree.stability[c], b = r.C.tree.stability[c];
      if (std::abs(a - b) > 1e-12 * std::max(std::abs(a), std::abs(b))) {
        ++r.stab_changed;
        if (b > 0 && std::isfinite(a / b)) r.stab_ratio_max = std::max(r.stab_ratio_max, a / b);
      }
    }
  if (r.V.tree.parent == r.C.tree.parent)
    for (u32 c = 0; c < r.clusters; ++c)
      if (!has_kid[c]) {
        r.leaf_sum_v += r.V.tree.stability[c];
        r.leaf_sum_c += r.C.tree.stability[c];
      }
  for (i32 l : r.V.label) r.noise_v += l < 0;
  for (i32 l : r.N.label) r.noise_n += l < 0;
  return r;
}

static bool load(const char* path, std::vector<u32>& x, std::vector<u32>& y, std::vector<u32>& z) {
  FILE* f = std::fopen(path, "rb");
  if (!f) return false;
  u32 buf[3];
  while (std::fread(buf, 4, 3, f) == 3) {
    x.push_back(buf[0]);
    y.push_back(buf[1]);
    z.push_back(buf[2]);
  }
  std::fclose(f);
  return true;
}

static void dump(const std::string& path, const Cloud& cloud, const std::vector<i32>& lab, u32 n) {
  std::vector<i32> out(n, -1);
  for (u32 s = 0; s < cloud.sites(); ++s)
    for (PointId p : cloud.ids.row(s)) out[idx(p)] = lab[s];
  FILE* o = std::fopen(path.c_str(), "wb");
  if (!o) return;
  std::fwrite(out.data(), 4, n, o);
  std::fclose(o);
}

static std::vector<ClusterParams> read_configs(const std::string& path) {
  std::vector<ClusterParams> list;
  FILE* c = std::fopen(path.c_str(), "r");
  if (!c) return list;
  unsigned long long m;
  double zz;
  char sel[16];
  int single;
  while (std::fscanf(c, "%llu %lf %15s %d", &m, &zz, sel, &single) == 4) {
    ClusterParams q;
    q.min_cluster_size = m;
    q.z = zz;
    q.selection = std::string(sel) == "leaf" ? Selection::leaf : Selection::eom;
    q.allow_single_cluster = single != 0;
    list.push_back(q);
  }
  std::fclose(c);
  return list;
}

static void report(const char* source, int k, size_t i, const ClusterParams& p, const PointDendrogram& d,
                   const l07::NormStats& ns, const Compare& r, double t_tower, double t_dendro) {
  std::printf("{\"source\":\"%s\",\"k\":%d,\"cfg\":%zu,\"mcs\":%llu,\"z\":%.17g,\"sel\":\"%s\",\"single\":%d,"
              "\"nodes\":%u,\"points\":%u,\"levels\":%zu,\"contracted\":%llu,\"relocated\":%llu,"
              "\"port_ok\":%d,\"clusters\":%u,\"leaves\":%u,\"sel_v\":%u,\"sel_c\":%u,\"sel_n\":%u,\"sel_vn\":%u,"
              "\"lab_vc\":%llu,\"lab_vn\":%llu,\"lab_cn\":%llu,\"lab_vvn\":%llu,\"selset_vc\":%d,\"selset_vn\":%d,"
              "\"stab_changed\":%llu,\"stab_ratio_max\":%.6g,\"leaf_sum_v\":%.9g,\"leaf_sum_c\":%.9g,\"trig_c\":%llu,\"trig_c_mass\":%llu,\"trig_n\":%llu,"
              "\"heavy\":%llu,\"noise_v\":%llu,\"noise_n\":%llu,\"depth\":%u,\"walk_deact\":%llu,\"walk_clab\":%llu,"
              "\"walk_plab\":%llu,\"t_tower\":%.4f,\"t_dendro\":%.4f,\"t_head_v\":%.5f,\"t_head_c\":%.5f}\n",
              source, k, i, (unsigned long long)p.min_cluster_size, p.z, p.selection == Selection::eom ? "eom" : "leaf",
              int(p.allow_single_cluster), d.nodes(), d.points(), d.level.size(), (unsigned long long)ns.contracted,
              (unsigned long long)ns.relocated, int(r.port_ok), r.clusters, r.leaves, r.sel_v, r.sel_c, r.sel_n, r.sel_vn,
              (unsigned long long)r.lab_vc, (unsigned long long)r.lab_vn, (unsigned long long)r.lab_cn,
              (unsigned long long)r.lab_vvn, int(r.selset_vc), int(r.selset_vn), (unsigned long long)r.stab_changed,
              r.stab_ratio_max, r.leaf_sum_v, r.leaf_sum_c, (unsigned long long)r.trig_c.nodes, (unsigned long long)r.trig_c.rest_mass,
              (unsigned long long)r.trig_n.nodes, (unsigned long long)r.trig_c.heavy, (unsigned long long)r.noise_v,
              (unsigned long long)r.noise_n, r.walk.max_depth, (unsigned long long)r.walk.deactivate,
              (unsigned long long)r.walk.cluster_label, (unsigned long long)r.walk.point_label, t_tower, t_dendro, r.t_v,
              r.t_c);
}

static int run(int argc, char** argv) {
  if (argc < 4) return 2;
  std::vector<int> klist{2};
  std::vector<std::string> entries{"core"}, mreach;
  std::string configs, dump_mode = "diff";
  unsigned threads = 1;
  for (int i = 4; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--k-list=", 0) == 0) klist = ints(a.substr(9));
    else if (a.rfind("--entries=", 0) == 0) entries = a.size() > 10 ? strs(a.substr(10)) : std::vector<std::string>{};
    else if (a.rfind("--mreach=", 0) == 0) mreach = strs(a.substr(9));
    else if (a.rfind("--configs=", 0) == 0) configs = a.substr(10);
    else if (a.rfind("--threads=", 0) == 0) threads = unsigned(std::stoul(a.substr(10)));
    else if (a.rfind("--dump=", 0) == 0) dump_mode = a.substr(7);
    else {
      std::fprintf(stderr, "option inconnue %s\n", a.c_str());
      return 2;
    }
  }
  std::vector<u32> x, y, z;
  if (!load(argv[2], x, y, z)) return 2;
  const u32 n = static_cast<u32>(x.size());
  std::vector<u32> pid(n);
  for (u32 i = 0; i < n; ++i) pid[i] = i;
  auto prepared = prepare_cloud(x, y, z, pid, kCoordinateBits);
  if (!prepared.ok()) {
    std::printf("{\"status\":\"refus\",\"reason\":\"%s\"}\n", std::string(reason_name(prepared.outcome().reason)).c_str());
    return 2;
  }
  const Cloud& cloud = prepared.value();
  sched::Pool pool(threads);
  SiteTree tree(cloud);
  const std::vector<ClusterParams> list = read_configs(configs);
  if (list.empty()) return 2;
  const std::string prefix = argv[3];
  int kmax = 0, max_extra = 0;
  for (int kk : klist) kmax = std::max(kmax, kk);
  for (const std::string& e : entries)
    if (e.size() == 6 && e.rfind("cover", 0) == 0) max_extra = std::max(max_extra, e[5] - '0');
  auto handle = [&](const std::string& source, int kk, const PointDendrogram& d, double t_tower, double t_dendro) {
    const Outcome v = validate(d);
    if (!v.ok()) {
      std::printf("{\"source\":\"%s\",\"k\":%d,\"status\":\"invalid\",\"reason\":\"%s\"}\n", source.c_str(), kk,
                  std::string(reason_name(v.reason)).c_str());
      return;
    }
    l07::NormStats ns;
    const PointDendrogram dn = l07::normalize(d, &ns);
    const Outcome vn = validate(dn);
    if (!vn.ok()) {
      std::printf("{\"source\":\"%s\",\"k\":%d,\"status\":\"invalid_normalized\",\"reason\":\"%s\"}\n", source.c_str(), kk,
                  std::string(reason_name(vn.reason)).c_str());
      return;
    }
    for (size_t i = 0; i < list.size(); ++i) {
      const Compare r = compare(d, dn, list[i]);
      report(source.c_str(), kk, i, list[i], d, ns, r, t_tower, t_dendro);
      const bool differ = r.lab_vn != 0 || r.lab_vc != 0;
      if (dump_mode == "all" || (dump_mode == "diff" && differ)) {
        const std::string base = prefix + "." + source + ".k" + std::to_string(kk) + "." + std::to_string(i);
        dump(base + ".v", cloud, r.V.label, n);
        dump(base + ".n", cloud, r.N.label, n);
        if (r.lab_cn != 0) dump(base + ".c", cloud, r.C.label, n);
      }
    }
    std::fflush(stdout);
  };
  if (!entries.empty()) {
    CatalogueParams catp;
    catp.kmax = kmax + max_extra;
    const auto c0 = Clock::now();
    auto cat = build_catalogue(cloud, catp, pool);
    if (!cat.ok()) {
      std::printf("{\"status\":\"%s\",\"reason\":\"%s\"}\n", std::string(status_name(cat.outcome().status())).c_str(),
                  std::string(reason_name(cat.outcome().reason)).c_str());
      return 2;
    }
    std::printf("{\"catalogue\":1,\"n\":%u,\"kmax\":%d,\"balls\":%u,\"t\":%.3f}\n", n, catp.kmax, cat.value().balls(),
                secs(c0, Clock::now()));
    for (int kk : klist)
      for (const std::string& e : entries) {
        TowerParams tp;
        tp.kmax = kmax;
        tp.only_order = kk;
        tp.entry = e == "core" ? PointEntry::core : PointEntry::cover;
        tp.cover_extra = e.size() == 6 ? e[5] - '0' : 0;
        const auto a0 = Clock::now();
        auto tw = build_tower(cloud, tree, cat.value(), tp, pool);
        if (!tw.ok()) {
          std::printf("{\"source\":\"%s\",\"k\":%d,\"status\":\"%s\",\"reason\":\"%s\"}\n", e.c_str(), kk,
                      std::string(status_name(tw.outcome().status())).c_str(),
                      std::string(reason_name(tw.outcome().reason)).c_str());
          continue;
        }
        if (int(tw.value().orders.size()) < kk) continue;
        const auto a1 = Clock::now();
        const PointDendrogram d = point_dendrogram(cat.value(), tw.value().orders[kk - 1], cloud);
        const auto a2 = Clock::now();
        handle(e, kk, d, secs(a0, a1), secs(a1, a2));
      }
  }
  for (const std::string& m : mreach) {
    const size_t c = m.find(':');
    const u64 alpha = std::stoull(m.substr(0, c));
    const bool border = m.substr(c + 1) == "border";
    for (int kk : klist) {
      const auto a0 = Clock::now();
      const PointDendrogram d = mreach_dendrogram(tree, u64(kk), pool, alpha, border);
      const auto a1 = Clock::now();
      handle("mr" + std::to_string(alpha) + (border ? "border" : "core"), kk, d, secs(a0, a1), 0.0);
    }
  }
  return 0;
}

static int search(int argc, char** argv) {
  int n = 6, grid = 16, dim = 3, k = 2, trials = 1000, show = 3;
  u64 seed = 1;
  std::string entry = "core";
  std::vector<int> mcs_list{2}, z_list{1};
  for (int i = 2; i < argc; ++i) {
    const std::string a = argv[i];
    if (a.rfind("--n=", 0) == 0) n = std::stoi(a.substr(4));
    else if (a.rfind("--grid=", 0) == 0) grid = std::stoi(a.substr(7));
    else if (a.rfind("--dim=", 0) == 0) dim = std::stoi(a.substr(6));
    else if (a.rfind("--k=", 0) == 0) k = std::stoi(a.substr(4));
    else if (a.rfind("--trials=", 0) == 0) trials = std::stoi(a.substr(9));
    else if (a.rfind("--seed=", 0) == 0) seed = std::stoull(a.substr(7));
    else if (a.rfind("--entry=", 0) == 0) entry = a.substr(8);
    else if (a.rfind("--mcs=", 0) == 0) mcs_list = ints(a.substr(6));
    else if (a.rfind("--z=", 0) == 0) z_list = ints(a.substr(4));
    else if (a.rfind("--show=", 0) == 0) show = std::stoi(a.substr(7));
    else return 2;
  }
  std::mt19937_64 rng(seed);
  sched::Pool pool(1);
  u64 built = 0, refused = 0, stab = 0, flips = 0, flips_noroot = 0, port_bad = 0, cn_diff = 0, shown = 0;
  u64 trig_cases = 0, nontrivial_norm = 0;
  for (int t = 0; t < trials; ++t) {
    std::set<std::array<u32, 3>> pts;
    while (int(pts.size()) < n) {
      std::array<u32, 3> p{0, 0, 0};
      for (int a = 0; a < dim; ++a) p[a] = u32(rng() % u64(grid));
      pts.insert(p);
    }
    std::vector<u32> x, y, z, pid;
    for (const auto& p : pts) {
      x.push_back(p[0]);
      y.push_back(p[1]);
      z.push_back(p[2]);
      pid.push_back(u32(pid.size()));
    }
    auto prepared = prepare_cloud(x, y, z, pid, kCoordinateBits);
    if (!prepared.ok()) {
      ++refused;
      continue;
    }
    const Cloud& cloud = prepared.value();
    SiteTree tree(cloud);
    CatalogueParams catp;
    const int extra = entry.size() == 6 ? entry[5] - '0' : 0;
    catp.kmax = k + extra;
    auto cat = build_catalogue(cloud, catp, pool);
    if (!cat.ok()) {
      ++refused;
      continue;
    }
    TowerParams tp;
    tp.kmax = k;
    tp.only_order = k;
    tp.entry = entry == "core" ? PointEntry::core : PointEntry::cover;
    tp.cover_extra = extra;
    auto tw = build_tower(cloud, tree, cat.value(), tp, pool);
    if (!tw.ok() || int(tw.value().orders.size()) < k) {
      ++refused;
      continue;
    }
    const PointDendrogram d = point_dendrogram(cat.value(), tw.value().orders[k - 1], cloud);
    if (!validate(d).ok()) {
      ++refused;
      continue;
    }
    ++built;
    l07::NormStats ns;
    const PointDendrogram dn = l07::normalize(d, &ns);
    nontrivial_norm += (ns.contracted + ns.relocated) != 0;
    for (int mcs : mcs_list)
      for (int zz : z_list)
        for (int single = 0; single < 2; ++single) {
          ClusterParams p;
          p.min_cluster_size = u64(mcs);
          p.z = zz;
          p.allow_single_cluster = single != 0;
          const Compare r = compare(d, dn, p);
          port_bad += !r.port_ok;
          trig_cases += r.trig_n.nodes != 0;
          stab += r.stab_changed != 0;
          cn_diff += r.lab_cn != 0;
          if (r.lab_vn != 0) {
            ++flips;
            flips_noroot += single == 0;
            if (shown < u64(show) || (single == 0 && shown < u64(2 * show))) {
              ++shown;
              std::printf("FLIP n=%d k=%d entry=%s mcs=%d z=%d single=%d contracted=%llu relocated=%llu sel_v=%u sel_n=%u pts=", n, k,
                          entry.c_str(), mcs, zz, single, (unsigned long long)ns.contracted,
                          (unsigned long long)ns.relocated, r.sel_v, r.sel_n);
              for (u32 i = 0; i < x.size(); ++i) std::printf("%s(%u,%u,%u)", i ? "," : "", x[i], y[i], z[i]);
              std::printf(" V=");
              std::vector<i32> lv(x.size()), ln(x.size());
              for (u32 s = 0; s < cloud.sites(); ++s)
                for (PointId q : cloud.ids.row(s)) {
                  lv[idx(q)] = r.V.label[s];
                  ln[idx(q)] = r.N.label[s];
                }
              for (i32 l : lv) std::printf("%d ", l);
              std::printf("N=");
              for (i32 l : ln) std::printf("%d ", l);
              std::printf("\n");
            }
          }
        }
  }
  std::printf("SEARCH n=%d grid=%d dim=%d k=%d entry=%s trials=%d built=%llu refused=%llu port_bad=%llu "
              "norm_nontrivial=%llu trig_cases=%llu stab_cases=%llu flips=%llu flips_root_excluded=%llu cn_diff=%llu\n",
              n, grid, dim, k, entry.c_str(), trials, (unsigned long long)built, (unsigned long long)refused,
              (unsigned long long)port_bad, (unsigned long long)nontrivial_norm, (unsigned long long)trig_cases,
              (unsigned long long)stab, (unsigned long long)flips, (unsigned long long)flips_noroot,
              (unsigned long long)cn_diff);
  return 0;
}

int main(int argc, char** argv) {
  if (argc < 2) return 2;
  const std::string mode = argv[1];
  if (mode == "run") return run(argc, argv);
  if (mode == "search") return search(argc, argv);
  return 2;
}
