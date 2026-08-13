// MorseHGP3D v3 — `WspdFrontLowerBound-v1`.
//
// Une seule passe de construction, une seule classification par `RectId`
// TERMINAL, un MASQUE DE LANES commun aux trois seuils — et non trois
// traversees. Le premier gain de constante nomme par l'audit est precisement
// une wavefront commune.
//
// Ce que le prefixe NE fait pas : il ne developpe aucun `PairId`, ne forme
// aucun atlas de cellules, ne construit aucune coface globale, et ne conclut
// jamais qu'un rectangle est vide de porteurs. Toute non-fermeture part en
// `CARRIER_FRONT`.
//
// Codes de sortie : 1 desaccord du juge, 2 campagne refusee AVANT calcul,
// 3 plancher ou invariant viole, 4 mutant tue.
#include <algorithm>
#include <array>
#include <cerrno>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <queue>
#include <string>
#include <vector>

#include "cloud_families.hpp"
#include "rect_front.hpp"
#include "wspd_front.hpp"

namespace {

using mhgp3v::RectBox;
using mhgp3v::RectLane;
using mhgp3v::RectVerdict;
using mhgp3v::WspdBox;

struct Node {
  RectBox box;
  int begin, end, left, right;
};

struct Tree {
  std::vector<Node> nodes;
  std::vector<std::array<long long, 3>> pts;
  std::vector<int> pid;    // PointId d'origine, conserve a travers les tris
};

WspdBox to_wspd(const RectBox& b) {
  WspdBox w{};
  for (int i = 0; i < 3; ++i) { w.lo[i] = b.lo[i]; w.hi[i] = b.hi[i]; }
  return w;
}

// Arbre a decoupe equitable. Tie-break final par `PointId` : l'identite du
// rectangle ne doit dependre ni de l'ordre d'entree, ni d'un predicat flottant.
int build(Tree* t, int b, int e, int leaf) {
  Node n{};
  n.begin = b; n.end = e; n.left = -1; n.right = -1;
  for (int i = 0; i < 3; ++i) { n.box.lo[i] = t->pts[b][i]; n.box.hi[i] = t->pts[b][i]; }
  for (int k = b; k < e; ++k)
    for (int i = 0; i < 3; ++i) {
      n.box.lo[i] = std::min(n.box.lo[i], t->pts[k][i]);
      n.box.hi[i] = std::max(n.box.hi[i], t->pts[k][i]);
    }
  const int id = (int)t->nodes.size();
  t->nodes.push_back(n);
  if (e - b > leaf) {
    int ax = 0; long long best = -1;
    for (int i = 0; i < 3; ++i) {
      const long long w = t->nodes[id].box.hi[i] - t->nodes[id].box.lo[i];
      if (w > best) { best = w; ax = i; }
    }
    std::vector<int> idx(e - b);
    for (int k = 0; k < e - b; ++k) idx[k] = b + k;
    const long long mid = (t->nodes[id].box.lo[ax] + t->nodes[id].box.hi[ax]) / 2;
    std::stable_sort(idx.begin(), idx.end(), [&](int u, int v) {
      const bool lu = t->pts[u][ax] <= mid, lv = t->pts[v][ax] <= mid;
      if (lu != lv) return lu;
      if (t->pts[u][ax] != t->pts[v][ax]) return t->pts[u][ax] < t->pts[v][ax];
      return t->pid[u] < t->pid[v];                       // tie-break par PointId
    });
    std::vector<std::array<long long, 3>> pb(e - b);
    std::vector<int> ib(e - b);
    for (int k = 0; k < e - b; ++k) { pb[k] = t->pts[idx[k]]; ib[k] = t->pid[idx[k]]; }
    for (int k = 0; k < e - b; ++k) { t->pts[b + k] = pb[k]; t->pid[b + k] = ib[k]; }
    int m = b;
    while (m < e && t->pts[m][ax] <= mid) ++m;
    if (m == b || m == e) m = (b + e) / 2;
    t->nodes[id].left = build(t, b, m, leaf);
    t->nodes[id].right = build(t, m, e, leaf);
  }
  return id;
}

struct Counters {
  long long terminals = 0, self_blocks = 0, classified = 0, evals = 0;
  long long closed[3] = {0, 0, 0}, carrier[3] = {0, 0, 0};
  long long mass_closed[3] = {0, 0, 0}, mass_carrier[3] = {0, 0, 0};
  long long central_all = 0, lambda_all = 0, none_hits = 0, mixed_hits = 0;
  long long eval_hwm = 0, over_quantum = 0, mass_total = 0;
};

const int kNeed[3] = {10, 9, 8};

// UNE SEULE descente, un MASQUE DE LANES PAR NŒUD — l'antichaine que l'audit
// specifie. Un nœud porte les lanes pour lesquelles il n'est pas encore
// resolu ; `ALL` credite et retire la lane, `NONE` la retire sans crediter,
// `MIXED` la transmet aux enfants. Aucun point n'est donc compte deux fois
// pour une meme lane, et aucune lane fermee ne continue de payer.
unsigned classify_rect(const Tree& t, int ia, int ib, long long quantum, Counters* c) {
  const Node& A = t.nodes[ia];
  const Node& B = t.nodes[ib];
  long long cred[3] = {0, 0, 0};
  unsigned closed_mask = 0, open = 7u;
  struct Item { long long pri; int node; unsigned mask; };
  struct Cmp { bool operator()(const Item& x, const Item& y) const { return x.pri < y.pri; } };
  std::priority_queue<Item, std::vector<Item>, Cmp> pq;
  long long used = 0;
  pq.push({(long long)1 << 62, 0, 7u});
  while (!pq.empty() && open && used < quantum) {
    const Item it = pq.top();
    pq.pop();
    const Node& C = t.nodes[it.node];
    const long long k = C.end - C.begin;
    ++used; ++c->evals;
    unsigned child = 0;
    long long mx_keep = 0;
    bool any_all = false;
    for (int q = 0; q < 3; ++q) {
      if (!(it.mask & open & (1u << q))) continue;
      long long mx = 0;
      const RectVerdict v = mhgp3v::rect_classify(A.box, B.box, C.box, (RectLane)q, &mx);
      if (v == RectVerdict::kAll) {
        any_all = true;
        cred[q] += k;
        if (cred[q] >= kNeed[q]) { closed_mask |= 1u << q; open &= ~(1u << q); }
      } else if (v == RectVerdict::kMixed) {
        child |= 1u << q;
        mx_keep = std::max(mx_keep, mx);
      } else {
        ++c->none_hits;
      }
    }
    if (any_all) ++c->central_all;
    if (child) {
      ++c->mixed_hits;
      if (C.left >= 0) {
        pq.push({mx_keep, C.left, child});
        pq.push({mx_keep, C.right, child});
      }
    }
  }
  if (used > c->eval_hwm) c->eval_hwm = used;
  if (used > quantum) ++c->over_quantum;
  ++c->classified;
  return closed_mask;
}

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

}  // namespace

int main(int argc, char** argv) {
  std::string family = "uniform";
  std::vector<long long> ns;
  long long coord = 65535, sep = 2, leaf = 1, seed = 12345, quantum = 64;
  bool oracle = false;
  long long min_closed = 0, min_carrier = 0;
  double max_slope = 1.35;

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto val = [&](const char* p) { return a.substr(std::strlen(p)); };
    if (a.rfind("--family=", 0) == 0) family = val("--family=");
    else if (a.rfind("--points=", 0) == 0) {
      std::string s = val("--points="); size_t p = 0;
      while (p < s.size()) { size_t q = s.find(',', p);
        if (q == std::string::npos) q = s.size();
        ns.push_back(arg_ll(s.substr(p, q - p).c_str(), 8, 100000000LL, "points")); p = q + 1; }
    }
    else if (a.rfind("--coord=", 0) == 0) coord = arg_ll(val("--coord=").c_str(), 8, 65535, "coord");
    else if (a.rfind("--sep=", 0) == 0) sep = arg_ll(val("--sep=").c_str(), 1, 32, "sep");
    else if (a.rfind("--leaf=", 0) == 0) leaf = arg_ll(val("--leaf=").c_str(), 1, 4096, "leaf");
    else if (a.rfind("--quantum=", 0) == 0) quantum = arg_ll(val("--quantum=").c_str(), 2, 1000000, "quantum");
    else if (a.rfind("--seed=", 0) == 0) seed = arg_ll(val("--seed=").c_str(), 0, (1LL << 40), "seed");
    else if (a.rfind("--min-closed=", 0) == 0) min_closed = arg_ll(val("--min-closed=").c_str(), 0, (1LL << 50), "min-closed");
    else if (a.rfind("--min-carrier=", 0) == 0) min_carrier = arg_ll(val("--min-carrier=").c_str(), 0, (1LL << 50), "min-carrier");
    else if (a.rfind("--max-slope=", 0) == 0) max_slope = std::atof(val("--max-slope=").c_str());
    else if (a == "--oracle") oracle = true;
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

  if (oracle && ns.back() > 64) refuse("l'oracle de multiplicite exige n <= 64");

  std::vector<double> front, tot_mass;
  Counters agg{};
  for (size_t k = 0; k < ns.size(); ++k) {
    const long long n = ns[k];
    const std::vector<mhgp::P3> cloud = mhgp3v::make_family_cloud(fam, (int)n, (int)coord, seed);
    if ((long long)cloud.size() * 100 < n * 90) refuse("cardinalite non garantie");
    Tree t;
    t.pts.reserve(cloud.size());
    t.pid.reserve(cloud.size());
    int next = 0;
    for (const mhgp::P3& p : cloud) { t.pts.push_back({p.x, p.y, p.z}); t.pid.push_back(next++); }
    // POSITIONS COINCIDENTES : refus explicite avant toute passe. La WSPD ne
    // separe jamais deux points confondus, et sa borne ne les couvre pas.
    {
      std::vector<std::array<long long, 3>> s = t.pts;
      std::sort(s.begin(), s.end());
      if (std::adjacent_find(s.begin(), s.end()) != s.end())
        refuse("positions coincidentes dans le nuage");
    }
    t.nodes.reserve(4 * t.pts.size());
    build(&t, 0, (int)t.pts.size(), (int)leaf);
    const long long m = (long long)t.pts.size();
    const long long total = m * (m - 1) / 2;

    // ---- PASSE 1 : la partition, sans aucune geometrie scientifique.
    std::vector<std::pair<int, int>> terms;
    std::vector<std::pair<int, int>> st{{0, 0}};
    while (!st.empty()) {
      const auto [ia, ib] = st.back(); st.pop_back();
      const Node& A = t.nodes[ia];
      const Node& B = t.nodes[ib];
      if (ia == ib) {
        if (A.left < 0) { if (A.end - A.begin > 1) ++agg.self_blocks; continue; }
        st.push_back({A.left, A.left}); st.push_back({A.right, A.right});
        st.push_back({A.left, A.right});
        continue;
      }
      if (mhgp3v::wspd_separated(to_wspd(A.box), to_wspd(B.box), sep)) {
        terms.push_back({ia, ib});
        continue;
      }
      const long long ra = mhgp3v::wspd_r2(to_wspd(A.box)), rb = mhgp3v::wspd_r2(to_wspd(B.box));
      if (A.left < 0 && B.left < 0) { terms.push_back({ia, ib}); continue; }
      if (A.left >= 0 && (B.left < 0 || ra >= rb)) {
        st.push_back({A.left, ib}); st.push_back({A.right, ib});
      } else {
        st.push_back({ia, B.left}); st.push_back({ia, B.right});
      }
    }

    // ---- ORACLE DE MULTIPLICITE. La somme des masses ne prouve rien : une
    // omission et un doublon de meme masse se compensent. On developpe donc
    // tous les records et on exige multiplicite EXACTEMENT UN par `PairId`.
    if (oracle) {
      std::map<std::pair<int, int>, int> mult;
      for (const auto& [ia, ib] : terms)
        for (int u = t.nodes[ia].begin; u < t.nodes[ia].end; ++u)
          for (int v = t.nodes[ib].begin; v < t.nodes[ib].end; ++v) {
            const int p = std::min(t.pid[u], t.pid[v]), q = std::max(t.pid[u], t.pid[v]);
            ++mult[{p, q}];
          }
      long long bad = 0;
      for (int p = 0; p < m; ++p)
        for (int q = p + 1; q < m; ++q) {
          auto it = mult.find({p, q});
          if (it == mult.end() || it->second != 1) ++bad;
        }
      std::printf("oracle n=%lld paires=%lld couvertes=%zu multiplicite_fautive=%lld\n",
                  m, total, mult.size(), bad);
      if (bad != 0) {
        std::fprintf(stderr, "INVARIANT VIOLE: %lld PairId de multiplicite differente de un\n", bad);
        return 3;
      }
      continue;
    }

    // ---- PASSE 2 : une classification par terminal, masque de lanes commun.
    Counters c{};
    for (const auto& [ia, ib] : terms) {
      const long long ka = t.nodes[ia].end - t.nodes[ia].begin;
      const long long kb = t.nodes[ib].end - t.nodes[ib].begin;
      const long long mass = ka * kb;
      c.mass_total += mass;
      const unsigned mask = classify_rect(t, ia, ib, quantum, &c);
      for (int q = 0; q < 3; ++q) {
        if (mask & (1u << q)) { ++c.closed[q]; c.mass_closed[q] += mass; }
        else { ++c.carrier[q]; c.mass_carrier[q] += mass; }
      }
    }
    c.terminals = (long long)terms.size();
    std::printf("n=%lld famille=%s sep=%lld quantum=%lld | front=%lld (%.3f/pt) self=%lld"
                " | q2 %.2f%% q3 %.2f%% q4 %.2f%% | carrier q2=%lld q3=%lld q4=%lld"
                " | eval=%lld hwm=%lld over=%lld ALL=%lld NONE=%lld MIXED=%lld masse=%lld/%lld\n",
                m, family.c_str(), sep, quantum, c.terminals, (double)c.terminals / (double)m,
                agg.self_blocks,
                100.0 * (double)c.mass_closed[0] / (double)total,
                100.0 * (double)c.mass_closed[1] / (double)total,
                100.0 * (double)c.mass_closed[2] / (double)total,
                c.carrier[0], c.carrier[1], c.carrier[2],
                c.evals, c.eval_hwm, c.over_quantum, c.central_all, c.none_hits, c.mixed_hits,
                c.mass_total, total);
    if (c.mass_total != total) {
      std::fprintf(stderr, "INVARIANT VIOLE: masse %lld != %lld\n", c.mass_total, total);
      return 3;
    }
    if (c.over_quantum != 0) {
      std::fprintf(stderr, "INVARIANT VIOLE: %lld rectangles au-dela du quantum\n", c.over_quantum);
      return 3;
    }
    front.push_back((double)c.terminals);
    tot_mass.push_back((double)c.mass_carrier[2]);
    for (int q = 0; q < 3; ++q) { agg.closed[q] += c.closed[q]; agg.carrier[q] += c.carrier[q]; }
  }
  if (oracle) { std::printf("oracle accord=OUI\n"); return 0; }

  int bad = 0;
  for (size_t k = 1; k < front.size(); ++k) {
    const double sf = std::log2(front[k] / front[k - 1]) / std::log2((double)ns[k] / (double)ns[k - 1]);
    std::printf("pente front_records=%.3f (%lld->%lld)\n", sf, ns[k - 1], ns[k]);
    if (sf >= max_slope) ++bad; else bad = 0;
    if (bad >= 2) {
      std::fprintf(stderr, "REFUS DE PENTE: deux pentes front_records consecutives >= %.2f\n", max_slope);
      return 3;
    }
  }
  long long tc = 0, tk = 0;
  for (int q = 0; q < 3; ++q) { tc += agg.closed[q]; tk += agg.carrier[q]; }
  if (tc < min_closed) { std::fprintf(stderr, "PLANCHER CLOSED: %lld < %lld\n", tc, min_closed); return 3; }
  if (tk < min_carrier) { std::fprintf(stderr, "PLANCHER CARRIER: %lld < %lld\n", tk, min_carrier); return 3; }
  std::printf("OK famille=%s sep=%lld CLOSED=%lld CARRIER=%lld\n", family.c_str(), sep, tc, tk);
  return 0;
}
