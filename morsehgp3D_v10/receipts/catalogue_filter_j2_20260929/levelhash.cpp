// Sonde hors depot : empreinte des niveaux exacts publies (cat.level, representation num / den) en ordre de rang,
// que le dump canonique n'ecrit pas (V4 du plan du juge). FNV-1a 64 bits sur (signe, mots) de num puis de den.
// Compilee contre la bibliotheque de chaque version : g++ -std=c++20 -O2 -I<src> levelhash.cpp <build>/libmhgp10_core.a
//   levelhash IN.u32le K THREADS
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
  for (u32 i = 0; i < n; ++i) {
    x[i] = raw[3 * i];
    y[i] = raw[3 * i + 1];
    z[i] = raw[3 * i + 2];
    pid[i] = i;
  }
  auto prepared = prepare_cloud(x, y, z, pid, kCoordinateBits);
  if (!prepared.ok()) return 2;
  CatalogueParams params;
  params.kmax = std::atoi(argv[2]);
  sched::Pool pool(unsigned(std::atoi(argv[3])));
  auto built = build_catalogue(prepared.value(), params, pool);
  if (!built.ok()) return 2;
  const Catalogue& cat = built.value();
  u64 h = 0xcbf29ce484222325ull;
  auto mix = [&](u64 v) {
    for (int b = 0; b < 8; ++b) {
      h ^= (v >> (8 * b)) & 0xff;
      h *= 0x100000001b3ull;
    }
  };
  for (const geom::Level& l : cat.level) {
    mix(l.num.neg);
    for (u64 w : l.num.w) mix(w);
    mix(l.den.neg);
    for (u64 w : l.den.w) mix(w);
  }
  std::printf("levels %zu hash %016llx\n", cat.level.size(), static_cast<unsigned long long>(h));
  return 0;
}
