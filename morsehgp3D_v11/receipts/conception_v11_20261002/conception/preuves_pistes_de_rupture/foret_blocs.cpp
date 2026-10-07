// Foret par ordre reconstruite depuis le dump du catalogue v10 (conception v11, pistes de rupture, 2 octobre 2026).
// But : mesurer quelle part des fusions de chaque ordre est LOCALE a un bloc spatial (tous les minima du
// sous-arbre dans le meme bloc de Morton), donc constructible par un Kruskal local, en parallele, sans noyau
// sequentiel global. Statistique en binaire64 (plus petites boules par force brute, tolerance relative) ; les
// coquilles etendues sont ignorees (quelques dizaines de boules) ; les plateaux ne sont pas regroupes (une
// fusion par jonction unissante). Ce n'est pas un juge : seulement des comptes pour une decision de conception.
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>
#include <vector>

using u32 = uint32_t;
using u64 = uint64_t;
struct V3 { double x, y, z; };
static inline V3 sub(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline double dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline V3 cross(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
static const double kEps = 1e-10;
static std::vector<V3> P;
static std::vector<u32> xs;  // sites tries par x
struct Ball { u32 q, p, flags; std::vector<u32> S, I, U; };
static u64 key3(u32 x, u32 y, u32 z) { return (u64(x) << 40) | (u64(y) << 20) | u64(z); }

static bool circum(const u32* s, int q, V3& c, double& r2) {
  const V3 a = P[s[0]];
  if (q == 2) {
    const V3 b = P[s[1]];
    c = {(a.x + b.x) / 2, (a.y + b.y) / 2, (a.z + b.z) / 2};
    const V3 d = sub(b, a);
    r2 = dot(d, d) / 4;
    return true;
  }
  if (q == 3) {
    const V3 u = sub(P[s[1]], a), v = sub(P[s[2]], a);
    const V3 n = cross(u, v);
    const double nn = dot(n, n);
    if (nn <= 0) return false;
    const double uu = dot(u, u), vv = dot(v, v);
    const V3 t1 = cross(v, n), t2 = cross(n, u);
    const V3 w = {(uu * t1.x + vv * t2.x) / (2 * nn), (uu * t1.y + vv * t2.y) / (2 * nn), (uu * t1.z + vv * t2.z) / (2 * nn)};
    c = {a.x + w.x, a.y + w.y, a.z + w.z};
    r2 = dot(w, w);
    return true;
  }
  const V3 u = sub(P[s[1]], a), v = sub(P[s[2]], a), w = sub(P[s[3]], a);
  const double det = dot(u, cross(v, w));
  if (det == 0) return false;
  const double uu = dot(u, u), vv = dot(v, v), ww = dot(w, w);
  const V3 t1 = cross(v, w), t2 = cross(w, u), t3 = cross(u, v);
  const V3 o = {(uu * t1.x + vv * t2.x + ww * t3.x) / (2 * det), (uu * t1.y + vv * t2.y + ww * t3.y) / (2 * det),
                (uu * t1.z + vv * t2.z + ww * t3.z) / (2 * det)};
  c = {a.x + o.x, a.y + o.y, a.z + o.z};
  r2 = dot(o, o);
  return true;
}

static int meb(const std::vector<u32>& F, V3& c, double& r2, std::array<u32, 4>& sup) {
  const int k = int(F.size());
  double best = 1e300;
  int bq = 0;
  auto consider = [&](const u32* s, int q) {
    V3 cc;
    double rr;
    if (!circum(s, q, cc, rr)) return;
    if (!(rr < best * (1 - kEps))) return;
    for (int i = 0; i < k; ++i) {
      const V3 d = sub(P[F[i]], cc);
      if (dot(d, d) > rr * (1 + kEps)) return;
    }
    best = rr;
    c = cc;
    bq = q;
    for (int i = 0; i < q; ++i) sup[i] = s[i];
  };
  u32 s[4];
  for (int a = 0; a < k; ++a)
    for (int b = a + 1; b < k; ++b) { s[0] = F[a]; s[1] = F[b]; consider(s, 2); }
  for (int a = 0; a < k; ++a)
    for (int b = a + 1; b < k; ++b)
      for (int d = b + 1; d < k; ++d) { s[0] = F[a]; s[1] = F[b]; s[2] = F[d]; consider(s, 3); }
  for (int a = 0; a < k; ++a)
    for (int b = a + 1; b < k; ++b)
      for (int d = b + 1; d < k; ++d)
        for (int e = d + 1; e < k; ++e) { s[0] = F[a]; s[1] = F[b]; s[2] = F[d]; s[3] = F[e]; consider(s, 4); }
  r2 = best;
  return bq;
}

static std::string keyof(std::vector<u32> v) {
  std::sort(v.begin(), v.end());
  return std::string(reinterpret_cast<const char*>(v.data()), v.size() * 4);
}
static u64 spread(u64 v) {  // 18 bits -> un bit sur trois
  u64 r = 0;
  for (int i = 0; i < 18; ++i) r |= ((v >> i) & 1) << (3 * i);
  return r;
}

int main(int argc, char** argv) {
  if (argc < 4) { std::fprintf(stderr, "usage: foret_blocs IN.u32le DUMP K\n"); return 2; }
  const int K = std::atoi(argv[3]);
  FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  std::unordered_map<u64, u32> idx;
  std::vector<u64> morton;
  u32 buf[3];
  while (std::fread(buf, 4, 3, f) == 3) {
    idx.emplace(key3(buf[0], buf[1], buf[2]), u32(P.size()));
    P.push_back({double(buf[0]), double(buf[1]), double(buf[2])});
    morton.push_back(spread(buf[0]) | (spread(buf[1]) << 1) | (spread(buf[2]) << 2));
  }
  std::fclose(f);
  const u32 n = u32(P.size());
  std::vector<u32> order(n), mrank(n);
  for (u32 i = 0; i < n; ++i) order[i] = i;
  std::sort(order.begin(), order.end(), [&](u32 a, u32 b) { return morton[a] < morton[b]; });
  for (u32 i = 0; i < n; ++i) mrank[order[i]] = i;
  xs.resize(n);
  for (u32 i = 0; i < n; ++i) xs[i] = i;
  std::sort(xs.begin(), xs.end(), [&](u32 a, u32 b) { return P[a].x < P[b].x; });
  std::vector<double> xsv(n);
  for (u32 i = 0; i < n; ++i) xsv[i] = P[xs[i]].x;
  std::vector<Ball> balls;
  {
    FILE* d = std::fopen(argv[2], "r");
    if (!d) return 2;
    std::vector<char> line(1 << 16);
    while (std::fgets(line.data(), int(line.size()), d)) {
      Ball b;
      char* s = line.data();
      unsigned rank, q, p, u, fl;
      int used = 0;
      if (std::sscanf(s, "%u %u %u %u %u |%n", &rank, &q, &p, &u, &fl, &used) != 5) return 3;
      b.q = q; b.p = p; b.flags = fl;
      s += used;
      int part = 0;
      while (*s) {
        while (*s == ' ') ++s;
        if (*s == '|') { ++part; ++s; continue; }
        if (*s == '\n' || !*s) break;
        unsigned x, y, z;
        int m = 0;
        if (std::sscanf(s, "%u,%u,%u%n", &x, &y, &z, &m) != 3) return 3;
        s += m;
        const u32 id = idx.at(key3(x, y, z));
        (part == 0 ? b.S : part == 1 ? b.I : b.U).push_back(id);
      }
      balls.push_back(std::move(b));
    }
    std::fclose(d);
  }
  const u32 nb = u32(balls.size());
  std::unordered_map<std::string, u32> pop2ball, sup2ball;
  pop2ball.reserve(nb * 2);
  sup2ball.reserve(nb * 2);
  for (u32 i = 0; i < nb; ++i) {
    const Ball& b = balls[i];
    if (b.flags & 1) continue;
    std::vector<u32> pop = b.I;
    pop.insert(pop.end(), b.U.begin(), b.U.end());
    if (int(pop.size()) <= K) pop2ball.emplace(keyof(pop), i);
    sup2ball.emplace(keyof(b.U), i);
  }
  const int NB = 3;
  const u32 per_block[NB] = {880, 96, 10};  // sites par bloc de Morton (trame entiere : ~52, ~480, ~4 600 blocs)
  std::printf("sites %u boules %u K %d ; blocs de Morton de %u, %u et %u sites\n", n, nb, K, per_block[0], per_block[1],
              per_block[2]);
  std::printf("ordre | minima | jonctions | jonctions unissantes | fusions (jonctions unissantes) | racines finales | "
              "morceaux non resolus | part des fusions locales a un bloc (880 / 96 / 10 sites) | part des jonctions "
              "dont tous les morceaux tombent dans des composantes locales au meme bloc\n");
  std::vector<u32> bid(nb, 0), cellcomp(nb, 0);
  u64 tot_f = 0, tot_pure[NB] = {0, 0, 0};
  u64 seed_links = 0, seed_same[NB] = {0, 0, 0};  // liens morceau -> naissance semee : meme bloc de Morton ?
  auto ball_block = [&](u32 i, int t) {
    u32 best = balls[i].U[0];
    for (u32 s2 : balls[i].U)
      if (mrank[s2] < mrank[best]) best = s2;
    return int(mrank[best] / per_block[t]);
  };
  for (int k = 1; k <= K; ++k) {
    // minima
    std::vector<u32> rep_site;  // un site representatif par minimum (pour le bloc)
    if (k == 1) {
      for (u32 i = 0; i < n; ++i) rep_site.push_back(i);
    } else {
      for (u32 i = 0; i < nb; ++i) {
        const Ball& b = balls[i];
        if ((b.flags & 1) || int(b.p + b.q) != k) continue;
        bid[i] = u32(rep_site.size());
        u32 best = b.U[0];
        for (u32 s : b.U)
          if (mrank[s] < mrank[best]) best = s;
        rep_site.push_back(best);
      }
    }
    const u32 nm = u32(rep_site.size());
    std::vector<u32> uf(nm);
    for (u32 i = 0; i < nm; ++i) uf[i] = i;
    auto find = [&](u32 x) {
      while (uf[x] != x) { uf[x] = uf[uf[x]]; x = uf[x]; }
      return x;
    };
    std::vector<int> blk[NB];
    for (int t = 0; t < NB; ++t) {
      blk[t].resize(nm);
      for (u32 i = 0; i < nm; ++i) blk[t][i] = int(mrank[rep_site[i]] / per_block[t]);
    }
    u64 junctions = 0, uniting = 0, fusions = 0, unresolved = 0, pure[NB] = {0, 0, 0}, internal[NB] = {0, 0, 0};
    for (u32 bi = 0; bi < nb; ++bi) {
      const Ball& b = balls[bi];
      if ((b.flags & 1) || int(b.p + b.q) - 1 != k) continue;
      ++junctions;
      std::vector<u32> pop = b.I;
      pop.insert(pop.end(), b.U.begin(), b.U.end());
      std::vector<u32> roots;
      for (u32 ui = 0; ui < b.U.size(); ++ui) {
        std::vector<u32> F;
        for (u32 s : pop)
          if (s != b.U[ui]) F.push_back(s);
        u32 node = ~u32(0);
        if (k == 1) {
          node = F[0];
        } else {
          auto it = pop2ball.find(keyof(F));
          if (it != pop2ball.end()) {
            node = bid[it->second];
            ++seed_links;
            for (int t = 0; t < NB; ++t) seed_same[t] += ball_block(bi, t) == ball_block(it->second, t);
          } else {
            for (int step = 0; step < 40; ++step) {
              V3 cu{};
              double ru2;
              std::array<u32, 4> sup{};
              const int qu = meb(F, cu, ru2, sup);
              const double r = std::sqrt(ru2) * (1 + 1e-9);
              const size_t lo = std::lower_bound(xsv.begin(), xsv.end(), cu.x - r) - xsv.begin();
              const size_t hi = std::upper_bound(xsv.begin(), xsv.end(), cu.x + r) - xsv.begin();
              std::vector<std::pair<double, u32>> in;
              int mu = 0;
              for (size_t j = lo; j < hi; ++j) {
                const u32 i = xs[j];
                const V3 d = sub(P[i], cu);
                const double dd = dot(d, d);
                if (dd < ru2 * (1 - kEps)) in.push_back({dd, i});
                else if (dd <= ru2 * (1 + kEps)) ++mu;
              }
              const int pu = int(in.size());
              if (pu + mu == int(F.size())) {
                auto jt = pop2ball.find(keyof(F));
                if (jt != pop2ball.end()) node = bid[jt->second];
                break;
              }
              if (pu >= k) {
                std::sort(in.begin(), in.end());
                F.clear();
                for (int i = 0; i < k; ++i) F.push_back(in[i].second);
              } else if (k >= pu + qu - 1) {
                std::vector<u32> sv(sup.begin(), sup.begin() + qu);
                auto jt = sup2ball.find(keyof(sv));
                if (jt != sup2ball.end()) node = cellcomp[jt->second];
                break;
              } else {
                F.clear();
                for (auto& e : in) F.push_back(e.second);
                for (int i = 0; i < k - pu; ++i) F.push_back(sup[i]);
              }
            }
          }
        }
        if (node == ~u32(0)) { ++unresolved; continue; }
        roots.push_back(find(node));
      }
      if (roots.empty()) continue;
      std::sort(roots.begin(), roots.end());
      roots.erase(std::unique(roots.begin(), roots.end()), roots.end());
      for (int t = 0; t < NB; ++t) {
        bool same = blk[t][roots[0]] >= 0;
        for (u32 r : roots) same = same && blk[t][r] == blk[t][roots[0]];
        if (same) ++internal[t];
      }
      if (roots.size() >= 2) {
        ++uniting;
        ++fusions;
        for (int t = 0; t < NB; ++t) {
          bool same = blk[t][roots[0]] >= 0;
          for (u32 r : roots) same = same && blk[t][r] == blk[t][roots[0]];
          if (same) ++pure[t];
          else blk[t][roots[0]] = -1;
        }
        for (size_t i = 1; i < roots.size(); ++i) uf[roots[i]] = roots[0];
      }
      cellcomp[bi] = roots[0];
    }
    u64 nroots = 0;
    for (u32 i = 0; i < nm; ++i) nroots += find(i) == i;
    std::printf("%d | %u | %llu | %llu | %llu | %llu | %llu | %.1f %% / %.1f %% / %.1f %% | %.1f %% / %.1f %% / %.1f %%\n", k,
                nm, (unsigned long long)junctions, (unsigned long long)uniting, (unsigned long long)fusions,
                (unsigned long long)nroots, (unsigned long long)unresolved, 100.0 * pure[0] / std::max<u64>(1, fusions),
                100.0 * pure[1] / std::max<u64>(1, fusions), 100.0 * pure[2] / std::max<u64>(1, fusions),
                100.0 * internal[0] / std::max<u64>(1, junctions), 100.0 * internal[1] / std::max<u64>(1, junctions),
                100.0 * internal[2] / std::max<u64>(1, junctions));
    tot_f += fusions;
    for (int t = 0; t < NB; ++t) tot_pure[t] += pure[t];
  }
  std::printf("tous ordres : fusions %llu ; locales %.1f %% / %.1f %% / %.1f %%\n", (unsigned long long)tot_f,
              100.0 * tot_pure[0] / tot_f, 100.0 * tot_pure[1] / tot_f, 100.0 * tot_pure[2] / tot_f);
  std::printf("liens de semis (morceau -> naissance) : %llu ; naissance dans le meme bloc de Morton que la jonction : "
              "%.1f %% / %.1f %% / %.1f %%\n",
              (unsigned long long)seed_links, 100.0 * seed_same[0] / seed_links, 100.0 * seed_same[1] / seed_links,
              100.0 * seed_same[2] / seed_links);
  return 0;
}
