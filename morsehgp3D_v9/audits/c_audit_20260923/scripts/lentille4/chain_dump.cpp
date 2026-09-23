// Pilote d'audit (lecture seule du produit) : execute run_tower_chain sur de
// petits nuages et imprime le catalogue recoupe en rangs d'entree.
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
#include "chain/tower_chain.hpp"

namespace {
std::string i128s(mhgp9::tower::i128 v) {
  if (v == 0) return "0";
  bool neg = v < 0;
  unsigned __int128 m = neg ? static_cast<unsigned __int128>(-(v + 1)) + 1 : static_cast<unsigned __int128>(v);
  std::string s;
  while (m) { s.push_back(char('0' + int(m % 10))); m /= 10; }
  if (neg) s.push_back('-');
  std::reverse(s.begin(), s.end());
  return s;
}
}  // namespace

int main() {
  char name[64];
  int n;
  while (std::scanf("%63s %d", name, &n) == 2) {
    std::vector<mhgp9::gen::Point3> pts(n);
    std::vector<mhgp9::tower::InputPoint> input(n);
    for (int i = 0; i < n; ++i) {
      int x, y, z;
      if (std::scanf("%d %d %d", &x, &y, &z) != 3) return 2;
      pts[i] = {x, y, z};
      input[i] = mhgp9::tower::InputPoint{static_cast<mhgp9::tower::PointId>(i), mhgp9::tower::P3{x, y, z}};
    }
    const auto ix = mhgp9::tower::build_cloud_index(input);
    for (unsigned s : {8u, 12u})
      for (std::size_t w : {std::size_t{1}, std::size_t{2}})
        for (unsigned k = 1; k <= 10; ++k) {
          mhgp9::ChainOptions o;
          o.kmax = k; o.separation_s = s; o.workers = w; o.tower_static_threads = static_cast<int>(w > 1 ? 2 : 0);
          o.run_tower = true; o.keep_catalogue = true;
          const auto r = mhgp9::run_tower_chain(pts, o);
          std::printf("RUN %s s=%u w=%zu k=%u status=%s reason=%s balls=%zu digest=%016llx\n", name, s, w, k,
                      mhgp9::chain_status_name(r.status), r.reason.c_str(), r.catalogue_balls.size(),
                      static_cast<unsigned long long>(r.tower_digest));
          for (const auto& b : r.catalogue_balls) {
            std::vector<unsigned> in, sh;
            for (auto u : b.interior()) in.push_back(ix.point_id(u));
            for (auto u : b.shell()) sh.push_back(ix.point_id(u));
            std::sort(in.begin(), in.end()); std::sort(sh.begin(), sh.end());
            std::printf("B %s %s %s %s %s q=%u I=", i128s(b.key.a).c_str(), i128s(b.key.b[0]).c_str(),
                        i128s(b.key.b[1]).c_str(), i128s(b.key.b[2]).c_str(), i128s(b.key.c).c_str(), b.arity);
            for (std::size_t j = 0; j < in.size(); ++j) std::printf("%s%u", j ? "," : "", in[j]);
            std::printf(" U=");
            for (std::size_t j = 0; j < sh.size(); ++j) std::printf("%s%u", j ? "," : "", sh[j]);
            std::printf("\n");
          }
        }
  }
  return 0;
}
