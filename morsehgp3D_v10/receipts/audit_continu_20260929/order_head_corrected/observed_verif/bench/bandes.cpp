// Verificateur adverse (hors depot) : structure des bandes du catalogue publie (base), pour la loi d'Amdahl de l'etage
// des bandes a P fils. Groupes d'egalite exacte (rangs egaux consecutifs) : plus gros groupe, et travail de tri exact
// attribue aux tranches de A1 (tranche = celle du debut du groupe, 8P tranches au plus, comme generator.cpp) contre la
// moyenne. Paires voisines de niveaux distincts mais proches (< 2^-40 relatif) : comptees.
//   bandes IN.u32le K P
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>

#include "catalogue/catalogue.hpp"

using namespace mhgp10;

int main(int argc, char** argv) {
  if (argc < 4) return 2;
  FILE* f = std::fopen(argv[1], "rb");
  if (!f) return 2;
  std::vector<u32> raw;
  u32 buf[3];
  while (std::fread(buf, 4, 3, f) == 3) raw.insert(raw.end(), buf, buf + 3);
  std::fclose(f);
  const u32 n = static_cast<u32>(raw.size() / 3);
  std::vector<u32> x(n), y(n), z(n), pid(n);
  for (u32 i = 0; i < n; ++i) x[i] = raw[3 * i], y[i] = raw[3 * i + 1], z[i] = raw[3 * i + 2], pid[i] = i;
  auto prepared = prepare_cloud(x, y, z, pid, kCoordinateBits);
  if (!prepared.ok()) return 2;
  CatalogueParams params;
  params.kmax = std::atoi(argv[2]);
  const u64 P = u64(std::atoi(argv[3]));
  sched::Pool pool(4);
  auto built = build_catalogue(prepared.value(), params, pool);
  if (!built.ok()) return 2;
  const Catalogue& cat = built.value();
  const u64 nb = cat.balls();
  const u64 chunks = std::min<u64>(P * 8, std::max<u64>(1, nb / 4096));
  std::vector<double> work(chunks, 0);
  u64 maxg = 0, groups = 0, members = 0;
  double total = 0;
  for (u64 i = 0; i < nb;) {
    u64 j = i + 1;
    while (j < nb && cat.rank[j] == cat.rank[i]) ++j;
    const u64 m = j - i;
    if (m > 1) {
      ++groups;
      members += m;
      maxg = std::max(maxg, m);
      const double w = double(m) * std::log2(double(m)) + double(m);  // tri exact + comparaisons des voisins
      work[i * chunks / nb] += w;
      total += w;
    }
    i = j;
  }
  // voisins de niveaux distincts dont les approximations sont a moins de 2^-40 en relatif (bandes non triviales)
  u64 near = 0;
  for (u64 r = 1; r < cat.level.size(); ++r) {
    const double a = cat.level[r - 1].approx(), b = cat.level[r].approx();
    if (b - a <= b * 0x1p-40) ++near;
  }
  const double mx = *std::max_element(work.begin(), work.end());
  std::printf("boules %llu groupes_egaux %llu membres %llu plus_gros %llu | P=%llu tranches %llu : travail max/moyen par "
              "tranche %.1f ; plus grosse tranche = %.2f %% du travail des bandes (borne serie) | niveaux voisins "
              "distincts a < 2^-40 : %llu\n",
              (unsigned long long)nb, (unsigned long long)groups, (unsigned long long)members,
              (unsigned long long)maxg, (unsigned long long)P, (unsigned long long)chunks, mx / (total / double(chunks)),
              100.0 * mx / total, (unsigned long long)near);
  return 0;
}
