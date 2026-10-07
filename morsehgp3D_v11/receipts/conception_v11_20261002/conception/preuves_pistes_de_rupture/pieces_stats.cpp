// Statistique des morceaux de jonction (conception v11, pistes de rupture, 2 octobre 2026).
// Lit le nuage u32le et le dump texte de mhgp10_catalogue (rang q p u flags | S* | I | U), puis mesure :
//   (a) la part du catalogue par somme s = p + q ;
//   (b) pour chaque jonction reguliere (ordre k = p + q - 1 >= 2), la part des morceaux P \ {u} qui sont la
//       population d'une naissance reguliere d'ordre k (semis) ;
//   (c) pour les morceaux hors semis, une descente ideale avec arret a la premiere cellule du catalogue :
//       nombre de pas, nature de chaque pas, spheres hors catalogue ;
//   (d) pour le premier pas : le recensement de la plus petite boule du morceau est-il certifie par N_K(c),
//       c le centre de la boule de jonction (condition verifiable dans la feuille du generateur) ?
// Geometrie en binaire64 avec tolerance relative : c'est une STATISTIQUE, pas une decision exacte.
// Les egalites d'ensembles (semis) sont exactes (identifiants de sites lus dans le dump exact de la v10).
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using u32 = uint32_t;
using u64 = uint64_t;
struct V3 { double x, y, z; };
static inline V3 sub(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
static inline double dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
static inline V3 cross(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
static const double kEps = 1e-10;

static std::vector<V3> P;  // sites

struct Ball { u32 q, p, flags; std::vector<u32> S, I, U; };

static u64 key3(u32 x, u32 y, u32 z) { return (u64(x) << 40) | (u64(y) << 20) | u64(z); }

// centre et rayon carre de la sphere circonscrite minimale de 2, 3 ou 4 points ; false si degenere
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
    // c - a = (uu (v x n) + vv (n x u)) / (2 nn)
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

// plus petite boule englobante de F (k <= 12) par force brute sur les supports de 2 a 4 points
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

int main(int argc, char** argv) {
  if (argc < 4) { std::fprintf(stderr, "usage: pieces_stats IN.u32le DUMP K [pas_echantillon]\n"); return 2; }
  const int K = std::atoi(argv[3]);
  const u32 stride = argc > 4 ? u32(std::atoi(argv[4])) : 1;
  FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  std::unordered_map<u64, u32> idx;
  u32 buf[3];
  while (std::fread(buf, 4, 3, f) == 3) {
    idx.emplace(key3(buf[0], buf[1], buf[2]), u32(P.size()));
    P.push_back({double(buf[0]), double(buf[1]), double(buf[2])});
  }
  std::fclose(f);
  const u32 n = u32(P.size());
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
  const size_t nb = balls.size();
  // (a) comptes par somme
  std::vector<u64> by_sum(K + 3, 0);
  u64 ext = 0;
  for (const Ball& b : balls) { ++by_sum[b.p + b.q]; ext += b.flags & 1; }
  std::printf("sites %u boules %zu K %d etendues %llu\n", n, nb, K, (unsigned long long)ext);
  for (int s = 2; s <= K + 1; ++s) std::printf("  somme p+q=%d : %llu\n", s, (unsigned long long)by_sum[s]);
  // tables : naissances regulieres par population ; cellules du catalogue par support
  std::unordered_set<std::string> birth_pop;
  birth_pop.reserve(nb * 2);
  for (const Ball& b : balls) {
    if (b.flags & 1) continue;
    std::vector<u32> pop = b.I;
    pop.insert(pop.end(), b.U.begin(), b.U.end());
    if (int(pop.size()) <= K) birth_pop.insert(keyof(pop));
  }
  // (b), (c), (d)
  u64 pieces = 0, pieces_k1 = 0, seeds = 0, nonseed = 0, analysed = 0;
  u64 steps_total = 0, jumps = 0, inert = 0, out_of_cat = 0, term_birth = 0, term_birth_missing = 0, term_cell = 0,
      term_cell_ext = 0, first_cell = 0, first_birth = 0, first_jump = 0, first_inert = 0, cert_nk = 0, cert_geo = 0,
      maxchain = 0, unresolved = 0;
  std::vector<u64> chain_hist(40, 0), pieces_by_k(K + 2, 0), nonseed_by_k(K + 2, 0), cert_by_k(K + 2, 0),
      ana_by_k(K + 2, 0), steps_by_k(K + 2, 0);
  double sum_disp_over_r = 0;
  std::vector<double> disp_ratio;
  std::vector<double> dist(n);
  u64 counter = 0;
  for (size_t bi = 0; bi < nb; ++bi) {
    const Ball& b = balls[bi];
    if (b.flags & 1) continue;  // coquille etendue : hors statistique
    const int k = int(b.p + b.q) - 1;
    if (k < 1 || k > K) continue;
    if (k == 1) { pieces_k1 += b.q; continue; }
    std::vector<u32> pop = b.I;
    pop.insert(pop.end(), b.U.begin(), b.U.end());
    bool have_c = false;
    V3 c{};
    double r2 = 0, dk2 = 0;
    for (u32 ui = 0; ui < b.U.size(); ++ui) {
      ++pieces;
      ++pieces_by_k[k];
      std::vector<u32> F;
      for (u32 s : pop)
        if (s != b.U[ui]) F.push_back(s);
      if (birth_pop.count(keyof(F))) { ++seeds; continue; }
      ++nonseed;
      ++nonseed_by_k[k];
      if ((counter++ % stride) != 0) continue;
      ++analysed;
      ++ana_by_k[k];
      if (!have_c) {
        circum(b.S.data(), int(b.q), c, r2);
        for (u32 i = 0; i < n; ++i) { const V3 d = sub(P[i], c); dist[i] = dot(d, d); }
        std::vector<double> tmp = dist;
        std::nth_element(tmp.begin(), tmp.begin() + (K - 1), tmp.end());
        dk2 = tmp[K - 1];
        have_c = true;
      }
      // descente ideale
      u64 chain = 0;
      bool first = true;
      for (;;) {
        V3 cu{};
        double ru2;
        std::array<u32, 4> sup{};
        const int qu = meb(F, cu, ru2, sup);
        ++chain;
        std::vector<std::pair<double, u32>> in;
        std::vector<u32> shell;
        bool all_in_nk = true;
        for (u32 i = 0; i < n; ++i) {
          const V3 d = sub(P[i], cu);
          const double dd = dot(d, d);
          if (dd < ru2 * (1 - kEps)) in.push_back({dd, i});
          else if (dd <= ru2 * (1 + kEps)) shell.push_back(i);
          else continue;
          if (first && dist[i] > dk2 * (1 + kEps)) all_in_nk = false;
        }
        const int pu = int(in.size()), mu = int(shell.size());
        if (first) {
          if (all_in_nk) { ++cert_nk; ++cert_by_k[k]; }
          const V3 dc = sub(cu, c);
          const double disp = std::sqrt(dot(dc, dc));
          if (std::sqrt(ru2) + disp <= std::sqrt(dk2) * (1 + kEps)) ++cert_geo;
          const double ratio = disp / std::sqrt(r2);
          sum_disp_over_r += ratio;
          disp_ratio.push_back(ratio);
        }
        const bool in_cat = pu + qu <= K + 1;
        if (!in_cat) ++out_of_cat;
        if (pu + mu == int(F.size())) {  // F est toute la population : naissance d'ordre k
          ++term_birth;
          if (!birth_pop.count(keyof(F))) ++term_birth_missing;  // naissance a coquille etendue, non semee
          if (first) ++first_birth;
          break;
        }
        if (pu >= k) {
          ++jumps;
          if (first) ++first_jump;
          std::sort(in.begin(), in.end());
          F.clear();
          for (int i = 0; i < k; ++i) F.push_back(in[i].second);
        } else if (k >= pu + qu - 1) {  // cellule de jonction d'ordre k au catalogue
          ++term_cell;
          if (mu > qu) ++term_cell_ext;
          if (first) ++first_cell;
          break;
        } else {  // inerte : I u (t sites du support), t = k - pu <= qu - 2
          ++inert;
          if (first) ++first_inert;
          F.clear();
          for (auto& e : in) F.push_back(e.second);
          for (int i = 0; i < k - pu; ++i) F.push_back(sup[i]);
        }
        first = false;
        if (chain > 30) { ++unresolved; break; }
      }
      steps_total += chain;
      steps_by_k[k] += chain;
      maxchain = std::max(maxchain, chain);
      ++chain_hist[std::min<u64>(chain, 39)];
    }
  }
  std::printf("morceaux k=1 (sites) %llu ; morceaux k>=2 %llu ; semis %llu (%.2f %%) ; hors semis %llu\n",
              (unsigned long long)pieces_k1, (unsigned long long)pieces, (unsigned long long)seeds,
              100.0 * seeds / pieces, (unsigned long long)nonseed);
  std::printf("hors semis analyses %llu (pas d'echantillon %u)\n", (unsigned long long)analysed, stride);
  std::printf("  pas de descente ideale : %llu (%.3f par morceau hors semis) ; chaine max %llu ; non resolus %llu\n",
              (unsigned long long)steps_total, double(steps_total) / analysed, (unsigned long long)maxchain,
              (unsigned long long)unresolved);
  std::printf("  premier pas : cellule de jonction du catalogue %llu (%.1f %%) ; naissance %llu (%.1f %%) ; saut %llu "
              "(%.1f %%) ; inerte %llu (%.1f %%)\n",
              (unsigned long long)first_cell, 100.0 * first_cell / analysed, (unsigned long long)first_birth,
              100.0 * first_birth / analysed, (unsigned long long)first_jump, 100.0 * first_jump / analysed,
              (unsigned long long)first_inert, 100.0 * first_inert / analysed);
  std::printf("  tous pas : sauts %llu ; inertes %llu ; spheres hors catalogue (p+q>K+1) %llu (%.1f %% des pas) ; "
              "arrets naissance %llu (dont hors table de semis %llu) ; arrets cellule %llu (dont coquille etendue %llu)\n",
              (unsigned long long)jumps, (unsigned long long)inert, (unsigned long long)out_of_cat,
              100.0 * out_of_cat / steps_total, (unsigned long long)term_birth, (unsigned long long)term_birth_missing,
              (unsigned long long)term_cell, (unsigned long long)term_cell_ext);
  std::printf("  premier pas certifie par N_K(c) : %llu (%.1f %%) ; par la condition r_u + |c-c_u| <= d_K(c) : %llu "
              "(%.1f %%)\n",
              (unsigned long long)cert_nk, 100.0 * cert_nk / analysed, (unsigned long long)cert_geo,
              100.0 * cert_geo / analysed);
  std::sort(disp_ratio.begin(), disp_ratio.end());
  if (!disp_ratio.empty())
    std::printf("  deplacement |c - c_u| / r : moyenne %.3f ; mediane %.3f ; p90 %.3f ; p99 %.3f\n",
                sum_disp_over_r / analysed, disp_ratio[disp_ratio.size() / 2], disp_ratio[disp_ratio.size() * 9 / 10],
                disp_ratio[disp_ratio.size() * 99 / 100]);
  std::printf("  histogramme des longueurs de chaine :");
  for (int i = 1; i < 40; ++i)
    if (chain_hist[i]) std::printf(" %d:%llu", i, (unsigned long long)chain_hist[i]);
  std::printf("\n  par ordre k : morceaux / hors semis (%%) / pas par hors-semis / premier pas certifie N_K (%%)\n");
  for (int k = 2; k <= K; ++k)
    std::printf("    k=%d : %llu / %llu (%.1f %%) / %.3f / %.1f %%\n", k, (unsigned long long)pieces_by_k[k],
                (unsigned long long)nonseed_by_k[k], 100.0 * nonseed_by_k[k] / std::max<u64>(1, pieces_by_k[k]),
                double(steps_by_k[k]) / std::max<u64>(1, ana_by_k[k]),
                100.0 * cert_by_k[k] / std::max<u64>(1, ana_by_k[k]));
  return 0;
}
