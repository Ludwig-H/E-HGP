// L04 audit : cout structurel des requetes de SiteTree sur une trame LiDAR (compteurs deterministes, copie instrumentee).
// Spheres : MEB de k plus proches voisins approches (paires, triangles non obtus, tetraedres a centre interieur) tirees
// parmi des voisins de Morton, comme les spheres que la tour interroge ; plus le k-NN d'un site (centre entier).
#include <cstdio>
#include <random>
#include <vector>

#include "cloud/site_tree.hpp"

extern unsigned long long l04_nodes, l04_leaves, l04_scanned, l04_exact, l04_shortcut, l04_cand;
using namespace mhgp10;

int main(int argc, char** argv) {
  if (argc < 2) return 2;
  FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  std::vector<u32> raw;
  u32 buf[3];
  while (std::fread(buf, 4, 3, f) == 3) raw.insert(raw.end(), buf, buf + 3);
  std::fclose(f);
  const u32 n = u32(raw.size() / 3);
  std::vector<u32> x(n), y(n), z(n), pid(n);
  for (u32 i = 0; i < n; ++i) {
    x[i] = raw[3 * i];
    y[i] = raw[3 * i + 1];
    z[i] = raw[3 * i + 2];
    pid[i] = i;
  }
  auto r = prepare_cloud(x, y, z, pid, 18);
  if (!r.ok()) return 2;
  const Cloud& c = r.value();
  SiteTree tree(c);
  auto P = [&](u32 s) { return geom::P3{i64(c.x[s]), i64(c.y[s]), i64(c.z[s])}; };
  std::mt19937_64 g(11);
  const u32 S = c.sites();
  // (1) k-NN d'un site, k = 5 et 10
  for (u32 k : {5u, 10u}) {
    l04_nodes = l04_leaves = l04_scanned = l04_exact = l04_cand = 0;
    std::vector<std::pair<i128, u32>> out;
    const u32 Q = 20000;
    for (u32 q = 0; q < Q; ++q) {
      const u32 s = u32(g() % S);
      tree.nearest(P(s), geom::Center{{0, 0, 0}, 1}, k, out);
    }
    std::printf("nearest centre=site k=%u : par requete noeuds %.1f feuilles %.1f sites balayes %.1f candidats %.1f cles exactes %.1f\n", k,
                double(l04_nodes) / Q, double(l04_leaves) / Q, double(l04_scanned) / Q, double(l04_cand) / Q, double(l04_exact) / Q);
  }
  // (2) boule fermee de spheres locales a centre dans l'enveloppe
  l04_nodes = l04_leaves = l04_scanned = l04_exact = l04_shortcut = 0;
  u64 Q = 0, inside = 0, shell = 0, cb[5] = {0, 0, 0, 0, 0};
  std::vector<u32> I, U;
  std::vector<std::pair<i128, u32>> nn;
  for (int t = 0; t < 60000; ++t) {
    const u32 s0 = u32(g() % S);
    tree.nearest(P(s0), geom::Center{{0, 0, 0}, 1}, 6, nn);  // voisins reels
    if (nn.size() < 6) continue;
    const u32 a = nn[0].second, b = nn[1 + g() % 5].second, d = nn[1 + g() % 5].second, e = nn[1 + g() % 5].second;
    const geom::P3 A = P(a), B = P(b), D = P(d), E = P(e);
    geom::Center ctr{};
    const u64 ar = g() % 3;
    if (ar == 0) {
      if (a == b) continue;
      geom::center2(A, B, ctr);
    } else if (ar == 1) {
      if (!geom::center3(A, B, D, ctr)) continue;
      if (geom::dot(geom::sub(B, A), geom::sub(D, A)) < 0 || geom::dot(geom::sub(A, B), geom::sub(D, B)) < 0 ||
          geom::dot(geom::sub(A, D), geom::sub(B, D)) < 0)
        continue;
    } else {
      if (!geom::center4(A, B, D, E, ctr)) continue;
      const geom::P3* tt[4] = {&A, &B, &D, &E};
      if (!geom::strictly_inside_tetra(tt, A, ctr)) continue;
    }
    const u64 b0 = l04_nodes, b1 = l04_leaves, b2 = l04_scanned, b3 = l04_exact, b4 = l04_shortcut;
    tree.closed_ball(A, ctr, I, U);
    cb[0] += l04_nodes - b0;
    cb[1] += l04_leaves - b1;
    cb[2] += l04_scanned - b2;
    cb[3] += l04_exact - b3;
    cb[4] += l04_shortcut - b4;
    ++Q;
    inside += I.size();
    shell += U.size();
  }
  // retirer le cout des nearest intercales : on remesure a part
  std::printf("closed_ball spheres locales (%llu) : interieur moyen %.2f coquille moyenne %.2f\n", (unsigned long long)Q, double(inside) / double(Q),
              double(shell) / double(Q));
  std::printf("  par sphere : noeuds %.1f feuilles %.1f sites balayes %.1f cles exactes %.2f raccourcis interieurs (sans cle exacte) %.2f\n",
              double(cb[0]) / double(Q), double(cb[1]) / double(Q), double(cb[2]) / double(Q), double(cb[3]) / double(Q), double(cb[4]) / double(Q));
  return 0;
}
