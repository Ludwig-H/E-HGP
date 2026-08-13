// MorseHGP3D v3 — PORTE DU FRONT DE RECTANGLES.
//
// Le sujet decide une lane pour un RECTANGLE de noeuds d'arbre, avec un budget
// BORNE d'evaluations par rectangle (raison de front `RESOURCE_CAP`). La
// recherche de temoins est AU MEILLEUR D'ABORD, ordonnee par `Lambda_max`
// decroissant — du noeud le plus interieur vers la frontiere : traiter la liste
// dans l'ordre de l'arbre epuise le budget sur la frontiere de la boule sans
// jamais atteindre le centre, et la fermeture est alors NULLE.
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
#include <numeric>
#include <queue>
#include <string>
#include <vector>

#include "cloud_families.hpp"
#include "rect_front.hpp"

namespace {

using mhgp3v::RectBox;
using mhgp3v::RectFrontInject;
using mhgp3v::RectVerdict;

struct Node {
  RectBox box;
  int begin, end, left, right;
  double rad;
};

struct Tree {
  std::vector<Node> nodes;
  std::vector<std::array<long long, 3>> pts;
};

int build(Tree* t, int b, int e, int leaf) {
  Node n{};
  n.begin = b; n.end = e; n.left = -1; n.right = -1;
  for (int i = 0; i < 3; ++i) { n.box.lo[i] = t->pts[b][i]; n.box.hi[i] = t->pts[b][i]; }
  for (int k = b; k < e; ++k)
    for (int i = 0; i < 3; ++i) {
      n.box.lo[i] = std::min(n.box.lo[i], t->pts[k][i]);
      n.box.hi[i] = std::max(n.box.hi[i], t->pts[k][i]);
    }
  double r2 = 0;
  for (int i = 0; i < 3; ++i) {
    const double h = 0.5 * (double)(n.box.hi[i] - n.box.lo[i]);
    r2 += h * h;
  }
  n.rad = std::sqrt(r2);
  const int id = (int)t->nodes.size();
  t->nodes.push_back(n);
  if (e - b > leaf) {
    int ax = 0; long long best = -1;
    for (int i = 0; i < 3; ++i) {
      const long long w = t->nodes[id].box.hi[i] - t->nodes[id].box.lo[i];
      if (w > best) { best = w; ax = i; }
    }
    const long long mid = (t->nodes[id].box.lo[ax] + t->nodes[id].box.hi[ax]) / 2;
    auto it = std::partition(t->pts.begin() + b, t->pts.begin() + e,
                             [ax, mid](const std::array<long long, 3>& p) { return p[ax] <= mid; });
    int m = (int)(it - t->pts.begin());
    if (m == b || m == e) {
      m = (b + e) / 2;
      std::nth_element(t->pts.begin() + b, t->pts.begin() + m, t->pts.begin() + e,
                       [ax](const std::array<long long, 3>& x, const std::array<long long, 3>& y) {
                         return x[ax] < y[ax];
                       });
    }
    t->nodes[id].left = build(t, b, m, leaf);
    t->nodes[id].right = build(t, m, e, leaf);
  }
  return id;
}

struct Counters {
  long long rect_visited = 0, rect_closed = 0, rect_residual = 0, rect_capped = 0;
  long long evals = 0, mass_closed = 0, mass_residual = 0, w_high = 0;
  long long all_hits = 0, none_hits = 0, mixed_hits = 0;
};

const int kNeed[3] = {10, 9, 8};

bool witness_enough(const Tree& t, int ia, int ib, int lane, long long budget,
                    RectFrontInject inject, Counters* c) {
  const Node& A = t.nodes[ia];
  const Node& B = t.nodes[ib];
  std::priority_queue<std::pair<long long, int>> pq;
  pq.push({(long long)1 << 62, 0});
  long long cred = 0;
  while (!pq.empty() && budget-- > 0) {
    const int ic = pq.top().second;
    pq.pop();
    const Node& C = t.nodes[ic];
    ++c->evals;
    long long mx = 0;
    const RectVerdict v = mhgp3v::rect_classify(A.box, B.box, C.box, lane, &mx, inject);
    if (v == RectVerdict::kNone) { ++c->none_hits; continue; }
    if (v == RectVerdict::kAll) {
      ++c->all_hits;
      cred += C.end - C.begin;
      if (cred >= kNeed[lane]) return true;
      continue;
    }
    ++c->mixed_hits;
    if (C.left >= 0) { pq.push({mx, C.left}); pq.push({mx, C.right}); }
    if ((long long)pq.size() > c->w_high) c->w_high = (long long)pq.size();
  }
  if (!pq.empty()) ++c->rect_capped;
  return false;
}

void solve(const Tree& t, int ia, int ib, int lane, long long budget,
           RectFrontInject inject, Counters* c) {
  const Node& A = t.nodes[ia];
  const Node& B = t.nodes[ib];
  const long long ka = A.end - A.begin, kb = B.end - B.begin;
  const long long mass = (ia == ib) ? ka * (ka - 1) / 2 : ka * kb;
  if (mass <= 0) return;
  ++c->rect_visited;
  if (ia == ib) {
    if (A.left < 0) { ++c->rect_residual; c->mass_residual += mass; return; }
    solve(t, A.left, A.left, lane, budget, inject, c);
    solve(t, A.right, A.right, lane, budget, inject, c);
    solve(t, A.left, A.right, lane, budget, inject, c);
    return;
  }
  if (witness_enough(t, ia, ib, lane, budget, inject, c)) {
    ++c->rect_closed; c->mass_closed += mass; return;
  }
  if (A.left < 0 && B.left < 0) { ++c->rect_residual; c->mass_residual += mass; return; }
  if (A.left >= 0 && (B.left < 0 || A.rad >= B.rad)) {
    solve(t, A.left, ib, lane, budget, inject, c);
    solve(t, A.right, ib, lane, budget, inject, c);
  } else {
    solve(t, ia, B.left, lane, budget, inject, c);
    solve(t, ia, B.right, lane, budget, inject, c);
  }
}

// ---- Juge independant : min et max exhaustifs sur les points ENTIERS des trois
// boites. Representation deliberement differente : aucune separation par
// coordonnee, une seule somme brute.
void brute_interval(const RectBox& a, const RectBox& b, const RectBox& cbx,
                    long long* mn, long long* mx) {
  bool first = true;
  for (long long ax = a.lo[0]; ax <= a.hi[0]; ++ax)
  for (long long ay = a.lo[1]; ay <= a.hi[1]; ++ay)
  for (long long az = a.lo[2]; az <= a.hi[2]; ++az)
  for (long long bx = b.lo[0]; bx <= b.hi[0]; ++bx)
  for (long long by = b.lo[1]; by <= b.hi[1]; ++by)
  for (long long bz = b.lo[2]; bz <= b.hi[2]; ++bz)
  for (long long zx = cbx.lo[0]; zx <= cbx.hi[0]; ++zx)
  for (long long zy = cbx.lo[1]; zy <= cbx.hi[1]; ++zy)
  for (long long zz = cbx.lo[2]; zz <= cbx.hi[2]; ++zz) {
    const long long v = (zx - ax) * (bx - zx) + (zy - ay) * (by - zy) + (zz - az) * (bz - zz);
    if (first) { *mn = *mx = v; first = false; }
    else { *mn = std::min(*mn, v); *mx = std::max(*mx, v); }
  }
}

struct SelfTest { long long tests = 0, disagree_min = 0, disagree_max = 0; };

SelfTest run_selftest(int iters, RectFrontInject inject) {
  SelfTest st{};
  unsigned long long r = 0x9E3779B97F4A7C15ull;
  auto rnd = [&r](long long hi) {
    r ^= r << 13; r ^= r >> 7; r ^= r << 17;
    return (long long)(r % (unsigned long long)hi);
  };
  for (int it = 0; it < iters; ++it) {
    RectBox bx[3];
    for (int q = 0; q < 3; ++q)
      for (int i = 0; i < 3; ++i) {
        const long long lo = rnd(11) - 5;
        bx[q].lo[i] = lo; bx[q].hi[i] = lo + rnd(3);
      }
    long long mn = 0, mx = 0, emn = 0, emx = 0;
    mhgp3v::rect_h_interval(bx[0], bx[1], bx[2], &mn, &mx, inject);
    brute_interval(bx[0], bx[1], bx[2], &emn, &emx);
    ++st.tests;
    if (mn != emn) ++st.disagree_min;
    if (mx != emx) ++st.disagree_max;
  }
  return st;
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
  long long coord = 65535, budget = 24, leaf = 8, seed = 12345, selftest = 20000;
  int lane = 0;
  double max_slope = 1.35;
  long long min_closed_pct = 0, min_all = 0, min_none = 0, min_mixed = 0;
  RectFrontInject inject = RectFrontInject::kNone;
  bool expect_mutant = false;

  for (int i = 1; i < argc; ++i) {
    const std::string a = argv[i];
    auto val = [&](const char* p) { return a.substr(std::strlen(p)); };
    if (a.rfind("--family=", 0) == 0) family = val("--family=");
    else if (a.rfind("--points=", 0) == 0) {
      std::string s = val("--points="); size_t p = 0;
      while (p < s.size()) { size_t q = s.find(',', p);
        if (q == std::string::npos) q = s.size();
        ns.push_back(arg_ll(s.substr(p, q - p).c_str(), 64, 100000000LL, "points")); p = q + 1; }
    }
    else if (a.rfind("--coord=", 0) == 0) coord = arg_ll(val("--coord=").c_str(), 16, 65535, "coord");
    else if (a.rfind("--budget=", 0) == 0) budget = arg_ll(val("--budget=").c_str(), 1, 1000000, "budget");
    else if (a.rfind("--leaf=", 0) == 0) leaf = arg_ll(val("--leaf=").c_str(), 1, 4096, "leaf");
    else if (a.rfind("--lane=", 0) == 0) lane = (int)arg_ll(val("--lane=").c_str(), 0, 2, "lane");
    else if (a.rfind("--seed=", 0) == 0) seed = arg_ll(val("--seed=").c_str(), 0, (1LL << 40), "seed");
    else if (a.rfind("--selftest=", 0) == 0) selftest = arg_ll(val("--selftest=").c_str(), 0, 1000000, "selftest");
    else if (a.rfind("--max-slope=", 0) == 0) max_slope = std::atof(val("--max-slope=").c_str());
    else if (a.rfind("--min-closed-pct=", 0) == 0) min_closed_pct = arg_ll(val("--min-closed-pct=").c_str(), 0, 100, "min-closed-pct");
    else if (a.rfind("--min-all=", 0) == 0) min_all = arg_ll(val("--min-all=").c_str(), 0, (1LL << 50), "min-all");
    else if (a.rfind("--min-none=", 0) == 0) min_none = arg_ll(val("--min-none=").c_str(), 0, (1LL << 50), "min-none");
    else if (a.rfind("--min-mixed=", 0) == 0) min_mixed = arg_ll(val("--min-mixed=").c_str(), 0, (1LL << 50), "min-mixed");
    else if (a == "--inject=max-par-coins") { inject = RectFrontInject::kMaxParCoins; expect_mutant = true; }
    else if (a == "--inject=sommet-non-ecrete") { inject = RectFrontInject::kSommetNonEcrete; expect_mutant = true; }
    else refuse("option inconnue");
  }
  if (ns.empty()) ns = {2000, 8000, 32000};

  // ---- Le juge d'abord : aucune mesure n'est publiee si l'intervalle est faux.
  if (selftest > 0) {
    const SelfTest st = run_selftest((int)selftest, inject);
    std::printf("selftest tests=%lld desaccords_min=%lld desaccords_max=%lld\n",
                st.tests, st.disagree_min, st.disagree_max);
    if (expect_mutant) {
      if (st.disagree_min == 0 && st.disagree_max == 0) {
        std::fprintf(stderr, "MUTANT SURVIVANT: l'injection ne contredit pas le juge\n");
        return 3;
      }
      std::printf("mutant_killed=1 raison=juge_exhaustif\n");
      return 4;
    }
    if (st.disagree_min != 0 || st.disagree_max != 0) {
      std::fprintf(stderr, "DESACCORD DU JUGE sur l'intervalle exact\n");
      return 1;
    }
  }

  mhgp3v::CloudFamily fam;
  if (family == "uniform") fam = mhgp3v::CloudFamily::kUniform;
  else if (family == "terrain") fam = mhgp3v::CloudFamily::kTerrain;
  else if (family == "eight_clusters") fam = mhgp3v::CloudFamily::kEightClusters;
  else if (family == "scanline_single_pass") fam = mhgp3v::CloudFamily::kScanlineSinglePass;
  else if (family == "scanline_overlap_multiecho") fam = mhgp3v::CloudFamily::kScanlineOverlapMultiecho;
  else refuse("famille inconnue");

  std::vector<double> resid_mass, visited_per_n;
  Counters agg{};
  for (size_t k = 0; k < ns.size(); ++k) {
    const long long n = ns[k];
    const std::vector<mhgp::P3> cloud = mhgp3v::make_family_cloud(fam, (int)n, (int)coord, seed);
    if ((long long)cloud.size() * 100 < n * 90)
      refuse("cardinalite non garantie par la famille");
    Tree t;
    t.pts.reserve(cloud.size());
    for (const mhgp::P3& p : cloud) t.pts.push_back({p.x, p.y, p.z});
    // condensat du nuage : la mesure est liee au nuage exact, pas a un nom
    unsigned long long digest = 1469598103934665603ull;
    for (const auto& p : t.pts)
      for (int i = 0; i < 3; ++i) { digest ^= (unsigned long long)p[i]; digest *= 1099511628211ull; }
    t.nodes.reserve(4 * t.pts.size());
    build(&t, 0, (int)t.pts.size(), (int)leaf);
    Counters c{};
    solve(t, 0, 0, lane, budget, inject, &c);
    const long long m = (long long)t.pts.size();
    const long long tot = m * (m - 1) / 2;
    if (c.mass_closed + c.mass_residual != tot) {
      std::fprintf(stderr, "INVARIANT VIOLE: partition des paires %lld + %lld != %lld\n",
                   c.mass_closed, c.mass_residual, tot);
      return 3;
    }
    std::printf("n=%lld famille=%s lane=q%d digest=%016llx | visites=%lld fermes=%lld residuels=%lld capes=%lld"
                " | masse_fermee=%lld masse_residuelle=%lld pct_ferme=%.2f"
                " | eval=%lld ALL=%lld NONE=%lld MIXED=%lld Wmax=%lld\n",
                m, family.c_str(), lane + 2, digest, c.rect_visited, c.rect_closed, c.rect_residual,
                c.rect_capped, c.mass_closed, c.mass_residual, 100.0 * (double)c.mass_closed / (double)tot,
                c.evals, c.all_hits, c.none_hits, c.mixed_hits, c.w_high);
    resid_mass.push_back((double)c.mass_residual);
    visited_per_n.push_back((double)c.rect_visited);
    agg.all_hits += c.all_hits; agg.none_hits += c.none_hits; agg.mixed_hits += c.mixed_hits;
    agg.mass_closed += c.mass_closed;
    if (k + 1 == ns.size() && min_closed_pct > 0 &&
        100.0 * (double)c.mass_closed / (double)tot < (double)min_closed_pct) {
      std::fprintf(stderr, "PLANCHER: fermeture %.2f%% < %lld%%\n",
                   100.0 * (double)c.mass_closed / (double)tot, min_closed_pct);
      return 3;
    }
  }

  int bad = 0;
  for (size_t k = 1; k < resid_mass.size(); ++k) {
    const double s = std::log2(resid_mass[k] / resid_mass[k - 1]) /
                     std::log2((double)ns[k] / (double)ns[k - 1]);
    const double sv = std::log2(visited_per_n[k] / visited_per_n[k - 1]) /
                      std::log2((double)ns[k] / (double)ns[k - 1]);
    std::printf("pente masse_residuelle=%.3f pente rect_visites=%.3f (%lld->%lld)\n",
                s, sv, ns[k - 1], ns[k]);
    if (s >= max_slope) ++bad; else bad = 0;
    if (bad >= 2) { std::fprintf(stderr, "REFUS DE PENTE: deux pentes consecutives >= %.2f\n", max_slope); return 3; }
  }
  if (agg.all_hits < min_all) { std::fprintf(stderr, "PLANCHER ALL: %lld < %lld\n", agg.all_hits, min_all); return 3; }
  if (agg.none_hits < min_none) { std::fprintf(stderr, "PLANCHER NONE: %lld < %lld\n", agg.none_hits, min_none); return 3; }
  if (agg.mixed_hits < min_mixed) { std::fprintf(stderr, "PLANCHER MIXED: %lld < %lld\n", agg.mixed_hits, min_mixed); return 3; }
  std::printf("OK famille=%s lane=q%d budget=%lld leaf=%lld ALL=%lld NONE=%lld MIXED=%lld\n",
              family.c_str(), lane + 2, budget, leaf, agg.all_hits, agg.none_hits, agg.mixed_hits);
  return 0;
}
