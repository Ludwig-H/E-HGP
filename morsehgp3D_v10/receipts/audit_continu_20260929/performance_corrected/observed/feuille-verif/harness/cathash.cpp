// Harnais du verificateur adverse (J3) : empreinte complete du catalogue, niveaux exacts compris.
// usage : cathash IN.u32le W K1,K2,... M1,M2,...   (M = 0 : taille de feuille par defaut)
// Une ligne par (K, M) : statut, grand livre complet, empreinte FNV-1a 64 de tous les tableaux publies.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "catalogue/catalogue.hpp"

using namespace mhgp10;

namespace {
struct Fnv {
  u64 h = 1469598103934665603ull;
  void add(const void* p, size_t n) {
    const unsigned char* c = static_cast<const unsigned char*>(p);
    for (size_t i = 0; i < n; ++i) {
      h ^= c[i];
      h *= 1099511628211ull;
    }
  }
  template <class T>
  void val(const T& v) {
    add(&v, sizeof(T));
  }
};
std::vector<int> parse_list(const char* s) {
  std::vector<int> r;
  std::string t(s);
  size_t a = 0;
  while (a <= t.size()) {
    size_t b = t.find(',', a);
    if (b == std::string::npos) b = t.size();
    if (b > a) r.push_back(std::atoi(t.substr(a, b - a).c_str()));
    a = b + 1;
  }
  return r;
}
}  // namespace

int main(int argc, char** argv) {
  if (argc < 5) {
    std::fprintf(stderr, "usage: cathash IN W K1,K2 M1,M2\n");
    return 2;
  }
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
  if (!prepared.ok()) {
    std::printf("prepare_refus %s\n", std::string(reason_name(prepared.outcome().reason)).c_str());
    return 0;
  }
  const Cloud& cloud = prepared.value();
  sched::Pool pool(unsigned(std::atoi(argv[2])));
  for (int K : parse_list(argv[3]))
    for (int M : parse_list(argv[4])) {
      CatalogueParams params;
      params.kmax = K;
      params.leaf_size = u32(M);
      auto built = build_catalogue(cloud, params, pool);
      if (!built.ok()) {
        std::printf("K=%d M=%d refus %s\n", K, M, std::string(reason_name(built.outcome().reason)).c_str());
        continue;
      }
      const Catalogue& cat = built.value();
      Fnv h;
      const u32 nb = cat.balls();
      for (u32 b = 0; b < nb; ++b) {
        h.val(cat.rank[b]);
        h.val(cat.support[b]);
        h.val(cat.qmin[b]);
        h.val(cat.p[b]);
        h.val(cat.u[b]);
        h.val(cat.flags[b]);
        h.val(cat.n_interior[b]);
        h.val(cat.pop_off[b]);
      }
      if (nb) h.val(cat.pop_off[nb]);
      for (u32 v : cat.pop) h.val(v);
      for (const auto& l : cat.level) {
        h.val(l.num.neg);
        for (int i = 0; i < 3; ++i) h.val(l.num.w[i]);
        h.val(l.den.neg);
        for (int i = 0; i < 2; ++i) h.val(l.den.w[i]);
      }
      const auto& L = cat.ledger;
      std::printf(
          "K=%d M=%d ok balls=%u levels=%zu hash=%016llx nodes=%llu leaves=%llu skipped=%llu sum_m=%llu max_m=%llu "
          "filter=%llu dom=%llu pair=%llu triple=%llu line=%llu quad=%llu judged=%llu emitted=%llu ext=%llu "
          "weighted=%llu max_shell=%llu stalled=%llu\n",
          K, M, nb, cat.level.size(), (unsigned long long)h.h, (unsigned long long)L.nodes,
          (unsigned long long)L.leaves, (unsigned long long)L.skipped_bbox, (unsigned long long)L.sum_m,
          (unsigned long long)L.max_m, (unsigned long long)L.filter_tests, (unsigned long long)L.leaf_dominance_tests,
          (unsigned long long)L.pair_tests, (unsigned long long)L.triple_tests, (unsigned long long)L.line_hits,
          (unsigned long long)L.quad_tests, (unsigned long long)L.judged, (unsigned long long)L.emitted,
          (unsigned long long)L.extended, (unsigned long long)L.weighted, (unsigned long long)L.max_shell,
          (unsigned long long)L.stalled_leaves);
    }
  return 0;
}
