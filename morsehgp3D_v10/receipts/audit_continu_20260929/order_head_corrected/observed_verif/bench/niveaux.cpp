// Verificateur adverse (hors depot) : empreinte des niveaux exacts publies (cat.level, num/den, signe et mots) en ordre
// de rang, et, si la bibliotheque publie level_approx (-DAVEC_APPROX), controle bit a bit level_approx[r] ==
// level[r].approx() pour tout r.   niveaux IN.u32le K FILS
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
  sched::Pool pool(unsigned(std::atoi(argv[3])));
  auto built = build_catalogue(prepared.value(), params, pool);
  if (!built.ok()) {
    std::printf("refus %s\n", std::string(reason_name(built.outcome().reason)).c_str());
    return 2;
  }
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
  long approx_bad = -1;
#ifdef AVEC_APPROX
  approx_bad = 0;
  if (cat.level_approx.size() != cat.level.size()) approx_bad = 1L << 40;
  else
    for (size_t r = 0; r < cat.level.size(); ++r) {
      const double a = cat.level[r].approx();
      if (std::memcmp(&a, &cat.level_approx[r], sizeof a) != 0) ++approx_bad;
    }
#endif
  std::printf("niveaux %zu empreinte %016llx approx_ecarts %ld\n", cat.level.size(), (unsigned long long)h, approx_bad);
  return 0;
}
