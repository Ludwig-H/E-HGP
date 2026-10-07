// L04 audit : morton3 (dispersion 21 bits par masques) contre un entrelacement bit a bit independant, et
// invariants de prepare_cloud sur une trame (ordre strict des cles, CSR, multiplicites, permutation, renumerotation).
#include <algorithm>
#include <cstdio>
#include <random>
#include <vector>

#include "cloud/cloud.hpp"

using namespace mhgp10;

static u64 ref_key(u32 x, u32 y, u32 z) {
  u64 k = 0;
  for (int b = 20; b >= 0; --b) {
    k = (k << 1) | ((z >> b) & 1u);
    k = (k << 1) | ((y >> b) & 1u);
    k = (k << 1) | ((x >> b) & 1u);
  }
  return k;
}

int main(int argc, char** argv) {
  std::mt19937_64 g(1);
  u64 bad = 0, checks = 0;
  const u32 lim = (1u << 21) - 1;
  for (int i = 0; i < 2000000; ++i) {
    u32 x = u32(g()) & lim, y = u32(g()) & lim, z = u32(g()) & lim;
    if (i % 5 == 0) x = (i % 2) ? lim : 0;
    if (i % 7 == 0) y = (i % 2) ? lim : (1u << (i % 21));
    if (i % 11 == 0) z = lim - (1u << (i % 21));
    bad += morton3(x, y, z) != ref_key(x, y, z);
    ++checks;
  }
  // au-dela de 21 bits la cle masque silencieusement (collision) : prepare_cloud doit refuser bits > 21
  const bool collide = morton3(1u << 21, 0, 0) == morton3(0, 0, 0);
  std::vector<u32> one{1u << 21}, zero{0}, id{0};
  const bool refused22 = prepare_cloud(one, zero, zero, id, 22).outcome().reason == Reason::parameter_out_of_range;
  const bool refused_dom = prepare_cloud(one, zero, zero, id, 21).outcome().reason == Reason::coordinate_out_of_domain;
  std::printf("morton3 : %llu controles, %llu desaccords ; collision a 2^21 = %d ; bits=22 refuse = %d ; hors domaine refuse = %d\n",
              (unsigned long long)checks, (unsigned long long)bad, int(collide), int(refused22), int(refused_dom));
  int rc = bad ? 1 : 0;
  if (argc > 1) {
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
      pid[i] = 4000000000u - 3u * i;  // PointId arbitraires, decroissants
    }
    // doublons : on recopie 5 % des points avec d'autres PointId
    const u32 extra = n / 20;
    for (u32 i = 0; i < extra; ++i) {
      const u32 j = u32(g() % n);
      x.push_back(x[j]);
      y.push_back(y[j]);
      z.push_back(z[j]);
      pid.push_back(7u + i);
    }
    const u32 m = u32(x.size());
    auto A = prepare_cloud(x, y, z, pid, 18);
    std::vector<u32> perm(m);
    for (u32 i = 0; i < m; ++i) perm[i] = i;
    std::shuffle(perm.begin(), perm.end(), g);
    std::vector<u32> x2(m), y2(m), z2(m), p2(m);
    for (u32 i = 0; i < m; ++i) {
      x2[i] = x[perm[i]];
      y2[i] = y[perm[i]];
      z2[i] = z[perm[i]];
      p2[i] = pid[perm[i]];
    }
    auto B = prepare_cloud(x2, y2, z2, p2, 18);
    if (!A.ok() || !B.ok()) return 2;
    const Cloud& a = A.value();
    const Cloud& b = B.value();
    bool ok = a.sites() == b.sites() && a.weight == m && a.ids.well_formed() && b.ids.well_formed();
    u64 wsum = 0;
    u32 maxw = 0;
    for (u32 s = 0; ok && s < a.sites(); ++s) {
      ok &= a.x[s] == b.x[s] && a.y[s] == b.y[s] && a.z[s] == b.z[s] && a.w[s] == b.w[s];
      ok &= a.ids.row(s).size() == a.w[s];
      for (u32 t = 0; ok && t < a.w[s]; ++t) ok &= a.ids.row(s)[t] == b.ids.row(s)[t] && (t == 0 || a.ids.row(s)[t - 1] < a.ids.row(s)[t]);
      if (s) ok &= ref_key(a.x[s - 1], a.y[s - 1], a.z[s - 1]) < ref_key(a.x[s], a.y[s], a.z[s]);
      wsum += a.w[s];
      maxw = std::max(maxw, a.w[s]);
    }
    ok &= wsum == m;
    std::printf("trame : points=%u (dont %u doublons ajoutes) sites=%u poids max=%u ; invariants (permutation, CSR, ordre strict) : %s\n",
                m, extra, a.sites(), maxw, ok ? "conformes" : "FAUX");
    if (!ok) rc = 1;
  }
  return rc;
}
