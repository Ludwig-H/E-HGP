// MorseHGP3D v3 — WSPD PAR VAGUES : le prototype dont le kernel GPU sera la
// transcription directe. Aucune recursion, aucune pile, aucune file par thread.
//
// Codes de sortie : 1 desaccord du juge, 2 refus avant calcul, 3 plancher ou
// invariant, 4 mutant tue.
#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "cloud_families.hpp"
#include <chrono>

#include "rect_front.hpp"
#include "wspd_front.hpp"
#include "wspd_wavefront.hpp"

namespace {

using mhgp3v::WfNode;

[[noreturn]] void refuse(const char* why) {
  std::fprintf(stderr, "REFUS: %s\n", why);
  std::exit(2);
}

long long arg_ll(const char* s, long long lo, long long hi, const char* name) {
  errno = 0;
  char* end = nullptr;
  const long long v = std::strtoll(s, &end, 10);
  if (errno != 0 || end == s || *end != '\0') refuse(name);
  if (v < lo || v > hi) refuse(name);
  return v;
}

struct Pair { int a, b; };

// Banque Morton bornee, fusionnee a l'emission : des qu'une paire est declaree
// terminale, le MEME thread calcule son `Dlo`, lit sa fenetre Morton et
// applique le masque central. Le rectangle n'est jamais materialise en memoire ;
// seuls les residuels sont compactes. C'est le gain de bande passante du
// kernel vise.
struct BankStat { long long reads = 0, recerts = 0, closed[3] = {0, 0, 0}; };

// Cellule de Morton (porte la borne) ou boite serree (front bien plus petit).
bool g_tight = false;
bool g_bank = false;
long long g_win = 32, g_bankl = 16;
long long g_warms = 0;

// Cellule d'un identifiant de nœud : negatif = feuille (le point lui-meme).
mhgp3v::WspdBox cell_of(const std::vector<WfNode>& nodes,
                        const std::vector<std::array<long long, 3>>& pts, int id) {
  mhgp3v::WspdBox w{};
  if (id < 0) {
    const auto& p = pts[-1 - id];
    for (int i = 0; i < 3; ++i) { w.lo[i] = p[i]; w.hi[i] = p[i]; }
  } else if (g_tight) {
    for (int i = 0; i < 3; ++i) { w.lo[i] = nodes[id].tlo[i]; w.hi[i] = nodes[id].thi[i]; }
  } else {
    for (int i = 0; i < 3; ++i) { w.lo[i] = nodes[id].lo[i]; w.hi[i] = nodes[id].hi[i]; }
  }
  return w;
}

long long count_of(const std::vector<WfNode>& nodes, int id) {
  return (id < 0) ? 1 : (nodes[id].last - nodes[id].first + 1);
}

}  // namespace

int main(int argc, char** argv) {
  std::string family = "uniform";
  std::vector<long long> ns;
  long long coord = 65535, p = 2, q = 1, seed = 12345;
  bool oracle = false;
  double max_slope = 1.35;

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto val = [&](const char* pre) { return a.substr(std::strlen(pre)); };
    if (a.rfind("--family=", 0) == 0) family = val("--family=");
    else if (a.rfind("--points=", 0) == 0) {
      std::string s = val("--points="); size_t o = 0;
      while (o < s.size()) { size_t c = s.find(',', o);
        if (c == std::string::npos) c = s.size();
        ns.push_back(arg_ll(s.substr(o, c - o).c_str(), 8, 100000000LL, "points")); o = c + 1; }
    }
    else if (a.rfind("--coord=", 0) == 0) coord = arg_ll(val("--coord=").c_str(), 8, 65535, "coord");
    else if (a.rfind("--sep-euclid=", 0) == 0) {
      const std::string v = val("--sep-euclid=");
      const size_t sl = v.find('/');
      if (sl == std::string::npos) refuse("sep-euclid attend p/q");
      p = arg_ll(v.substr(0, sl).c_str(), 1, 64, "sep p");
      q = arg_ll(v.substr(sl + 1).c_str(), 1, 16, "sep q");
    }
    else if (a.rfind("--seed=", 0) == 0) seed = arg_ll(val("--seed=").c_str(), 0, (1LL << 40), "seed");
    else if (a.rfind("--max-slope=", 0) == 0) max_slope = std::atof(val("--max-slope=").c_str());
    else if (a == "--oracle") oracle = true;
    else if (a == "--tight") g_tight = true;
    else if (a == "--bank") g_bank = true;
    else if (a.rfind("--window=", 0) == 0) { g_win = arg_ll(val("--window=").c_str(), 2, 1024, "window"); g_bank = true; }
    else if (a.rfind("--bank-l=", 0) == 0) { g_bankl = arg_ll(val("--bank-l=").c_str(), 1, 64, "bank-l"); g_bank = true; }
    else if (a.rfind("--warms=", 0) == 0) g_warms = arg_ll(val("--warms=").c_str(), 1, 200, "warms");
    else refuse("option inconnue");
  }
  if (ns.empty()) ns = {4000, 16000};

  mhgp3v::CloudFamily fam;
  if (family == "uniform") fam = mhgp3v::CloudFamily::kUniform;
  else if (family == "terrain") fam = mhgp3v::CloudFamily::kTerrain;
  else if (family == "eight_clusters") fam = mhgp3v::CloudFamily::kEightClusters;
  else if (family == "scanline_single_pass") fam = mhgp3v::CloudFamily::kScanlineSinglePass;
  else if (family == "scanline_overlap_multiecho") fam = mhgp3v::CloudFamily::kScanlineOverlapMultiecho;
  else refuse("famille inconnue");
  if (oracle && ns.back() > 64) refuse("l'oracle exige n <= 64");

  std::vector<double> fronts;
  for (size_t k = 0; k < ns.size(); ++k) {
    const std::vector<mhgp::P3> cloud = mhgp3v::make_family_cloud(fam, (int)ns[k], (int)coord, seed);
    std::vector<std::array<long long, 3>> pts;
    for (const mhgp::P3& pt : cloud) pts.push_back({pt.x, pt.y, pt.z});
    // Tri Morton avec depart par PointId : l'ordre est UNIQUE, donc l'arbre
    // aussi, donc les `RectId` sont identiques sur CPU et sur device.
    std::vector<int> pid(pts.size());
    for (size_t i = 0; i < pts.size(); ++i) pid[i] = (int)i;
    std::sort(pid.begin(), pid.end(), [&](int u, int v) {
      const unsigned long long ku = mhgp3v::wf_morton48(pts[u][0], pts[u][1], pts[u][2]);
      const unsigned long long kv = mhgp3v::wf_morton48(pts[v][0], pts[v][1], pts[v][2]);
      if (ku != kv) return ku < kv;
      return u < v;
    });
    std::vector<std::array<long long, 3>> sp(pts.size());
    std::vector<int> spid(pts.size());
    for (size_t i = 0; i < pid.size(); ++i) { sp[i] = pts[pid[i]]; spid[i] = pid[i]; }
    std::vector<unsigned long long> keys(sp.size());
    for (size_t i = 0; i < sp.size(); ++i) keys[i] = mhgp3v::wf_morton48(sp[i][0], sp[i][1], sp[i][2]);
    {
      std::vector<unsigned long long> s2 = keys;
      if (std::adjacent_find(s2.begin(), s2.end()) != s2.end())
        refuse("codes de Morton coincidents : positions dupliquees");
    }
    // ---- CHRONO. Le contrat ne se decide pas sur des comptages. On mesure
    // l'arbre puis la vague fusionnee, sur `warms` repetitions, et on publie la
    // MEDIANE et le p95 — jamais la meilleure.
    using clk = std::chrono::steady_clock;
    std::vector<double> t_tree, t_wave;
    // L'arbre est repete `warms` fois ; la vague, une fois par execution du
    // probe. Le `p95` de la vague s'obtient en repetant le PROBE, ce qui mesure
    // aussi le cout froid — c'est plus honnete qu'une boucle chaude interne.
    const long long warms = std::max(1LL, g_warms);
    std::vector<WfNode> nodes;
    for (long long w = 0; w < warms; ++w) {
      const auto a0 = clk::now();
      nodes = mhgp3v::wf_build(keys);
      mhgp3v::wf_tight_boxes(&nodes, sp);
      t_tree.push_back(std::chrono::duration<double, std::milli>(clk::now() - a0).count());
    }
    const auto w0 = clk::now();
    const long long m = (long long)sp.size();

    // ---- GRAINES : le cas diagonal DEROULE. Un thread par nœud interne.
    std::vector<Pair> wave;
    wave.reserve(nodes.size());
    for (size_t i = 0; i < nodes.size(); ++i) wave.push_back({nodes[i].left, nodes[i].right});

    // ---- VAGUES : `count -> scan -> fill`, aucune pile.
    std::vector<Pair> terms;
    BankStat bank;
    long long tests = 0, levels = 0, wave_hwm = (long long)wave.size();
    while (!wave.empty()) {
      ++levels;
      std::vector<int> cnt(wave.size());
      std::vector<char> sep(wave.size());
      for (size_t i = 0; i < wave.size(); ++i) {
        ++tests;
        const mhgp3v::WspdBox ca = cell_of(nodes, sp, wave[i].a);
        const mhgp3v::WspdBox cb = cell_of(nodes, sp, wave[i].b);
        sep[i] = mhgp3v::wspd_separated_euclid(ca, cb, p, q) ? 1 : 0;
        if (sep[i]) { cnt[i] = 0; continue; }
        const bool la = wave[i].a < 0, lb = wave[i].b < 0;
        cnt[i] = (la && lb) ? 0 : 2;    // deux feuilles non separables : terminal force
      }
      std::vector<int> off(wave.size() + 1, 0);
      for (size_t i = 0; i < wave.size(); ++i) off[i + 1] = off[i] + cnt[i];
      std::vector<Pair> next(off.back());
      for (size_t i = 0; i < wave.size(); ++i) {
        if (cnt[i] == 0) {
          terms.push_back(wave[i]);
          if (g_bank) {
            const mhgp3v::WspdBox ba = cell_of(nodes, sp, wave[i].a);
            const mhgp3v::WspdBox bb = cell_of(nodes, sp, wave[i].b);
            mhgp3v::RectBox qa{}, qb{};
            long long m4[3];
            for (int d = 0; d < 3; ++d) {
              qa.lo[d] = ba.lo[d]; qa.hi[d] = ba.hi[d];
              qb.lo[d] = bb.lo[d]; qb.hi[d] = bb.hi[d];
              m4[d] = ba.lo[d] + ba.hi[d] + bb.lo[d] + bb.hi[d];
            }
            const long long dlo = mhgp3v::rect_minsq(qa, qb);   // UNE fois
            const unsigned long long qk =
                mhgp3v::wf_morton48(m4[0] / 4, m4[1] / 4, m4[2] / 4);
            size_t pos = (size_t)(std::lower_bound(keys.begin(), keys.end(), qk) - keys.begin());
            const size_t beg = (pos > (size_t)(g_win / 2)) ? pos - g_win / 2 : 0;
            const size_t end = std::min(keys.size(), beg + (size_t)g_win);
            long long cred[3] = {0, 0, 0};
            long long taken = 0;
            for (size_t r = beg; r < end && taken < g_bankl; ++r) {
              ++bank.reads;
              mhgp3v::RectBox zb{};
              for (int d = 0; d < 3; ++d) { zb.lo[d] = sp[r][d]; zb.hi[d] = sp[r][d]; }
              ++taken; ++bank.recerts;
              const unsigned got = mhgp3v::rect_central_mask_dlo(dlo, qa, qb, zb);
              for (int lane = 0; lane < 3; ++lane)
                if (got & (1u << lane)) ++cred[lane];
            }
            const int need[3] = {10, 9, 8};
            for (int lane = 0; lane < 3; ++lane)
              if (cred[lane] >= need[lane]) ++bank.closed[lane];
          }
          continue;
        }
        const int ia = wave[i].a, ib = wave[i].b;
        const long long ra = (ia < 0) ? 0 : mhgp3v::wspd_w2(cell_of(nodes, sp, ia));
        const long long rb = (ib < 0) ? 0 : mhgp3v::wspd_w2(cell_of(nodes, sp, ib));
        int o = off[i];
        if (ia >= 0 && (ib < 0 || ra >= rb)) {
          next[o++] = {nodes[ia].left, ib};
          next[o++] = {nodes[ia].right, ib};
        } else {
          next[o++] = {ia, nodes[ib].left};
          next[o++] = {ia, nodes[ib].right};
        }
      }
      wave.swap(next);
      wave_hwm = std::max(wave_hwm, (long long)wave.size());
    }

    t_wave.push_back(std::chrono::duration<double, std::milli>(clk::now() - w0).count());
    auto pct = [](std::vector<double> v, double f) {
      std::sort(v.begin(), v.end());
      return v[std::min(v.size() - 1, (size_t)(f * (double)v.size()))];
    };
    long long mass = 0;
    for (const Pair& t : terms) mass += count_of(nodes, t.a) * count_of(nodes, t.b);
    const long long total = m * (m - 1) / 2;

    if (oracle) {
      std::map<std::pair<int, int>, int> mult;
      long long diag = 0;
      for (const Pair& t : terms) {
        const int fa = (t.a < 0) ? (-1 - t.a) : nodes[t.a].first;
        const int la = (t.a < 0) ? (-1 - t.a) : nodes[t.a].last;
        const int fb = (t.b < 0) ? (-1 - t.b) : nodes[t.b].first;
        const int lb = (t.b < 0) ? (-1 - t.b) : nodes[t.b].last;
        for (int u = fa; u <= la; ++u)
          for (int v = fb; v <= lb; ++v) {
            if (spid[u] == spid[v]) { ++diag; continue; }
            ++mult[{std::min(spid[u], spid[v]), std::max(spid[u], spid[v])}];
          }
      }
      long long dup = 0, miss = 0;
      for (const auto& e : mult) if (e.second != 1) ++dup;
      for (int x = 0; x < m; ++x)
        for (int y = x + 1; y < m; ++y) if (!mult.count({x, y})) ++miss;
      std::printf("oracle n=%lld attendu=%lld cles=%zu diagonales=%lld doublons=%lld manquantes=%lld\n",
                  m, total, mult.size(), diag, dup, miss);
      if (diag || (long long)mult.size() != total || dup || miss) {
        std::fprintf(stderr, "INVARIANT VIOLE: la vague ne partitionne pas les paires\n");
        return 3;
      }
      continue;
    }

    std::printf("n=%lld famille=%s boite=%s sep=%lld/%lld | front=%zu (%.3f/pt) | vagues=%lld"
                " tests=%lld tests/front=%.2f vague_max=%lld | masse=%lld/%lld"
                " | arbre_med=%.1f ms arbre_p95=%.1f ms vague=%.1f ms"
                " | banque lectures=%lld recert=%lld ferme q2=%lld q3=%lld q4=%lld\n",
                m, family.c_str(), g_tight ? "serree" : "cellule", p, q, terms.size(), (double)terms.size() / (double)m,
                levels, tests, (double)tests / (double)terms.size(), wave_hwm, mass, total,
                pct(t_tree, 0.5), pct(t_tree, 0.95), t_wave.back(),
                bank.reads, bank.recerts, bank.closed[0], bank.closed[1], bank.closed[2]);
    if (mass != total) {
      std::fprintf(stderr, "INVARIANT VIOLE: masse %lld != %lld\n", mass, total);
      return 3;
    }
    fronts.push_back((double)terms.size());
  }
  if (oracle) { std::printf("oracle accord=OUI\n"); return 0; }
  int bad = 0;
  for (size_t k = 1; k < fronts.size(); ++k) {
    const double s = std::log2(fronts[k] / fronts[k - 1]) / std::log2((double)ns[k] / (double)ns[k - 1]);
    std::printf("pente front_records=%.3f (%lld->%lld)\n", s, ns[k - 1], ns[k]);
    if (s >= max_slope) ++bad; else bad = 0;
    if (bad >= 2) { std::fprintf(stderr, "REFUS DE PENTE: deux pentes >= %.2f\n", max_slope); return 3; }
  }
  std::printf("OK famille=%s sep=%lld/%lld\n", family.c_str(), p, q);
  return 0;
}
