// Auditeur v8 (14 sept. 2026) — histogramme des tailles de facteurs des
// rectangles TERMINAUX du front WSPD pur v4 (wspd_wavefront reproduit comme
// dans morsehgp3D_v4/bench/wspd_scaling_probe.cpp, sans elimination par
// temoins). Mesure de REGIME pour situer les tranches P0 v8 : compte de
// rectangles, somme des tailles de facteurs, plus grand facteur, masse de
// paires par classe de max(|A|,|B|). Aucune geometrie HGP decidee ici.
// Option --split=level : scinder la plus grande cellule de Morton (prefixe
// commun le plus court) au lieu du plus grand diametre de boite serree.
#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
#include "../../../morsehgp3D_v4/src/cloud/families.hpp"
#include "../../../morsehgp3D_v4/src/tree/radix_tree.hpp"
#include "../../../morsehgp3D_v4/src/wspd/wavefront.hpp"
using namespace mhgp4;
int main(int argc, char** argv) {
  CloudFamily fam = CloudFamily::kUniform; int n = 8000; i64 s = 8; long long seed = 3; bool level_split = false;
  for (int i = 1; i < argc; ++i) {
    std::string a = argv[i];
    if (a.rfind("--family=", 0) == 0) { std::string f = a.substr(9);
      if (f == "uniform") fam = CloudFamily::kUniform; else if (f == "terrain") fam = CloudFamily::kTerrain;
      else if (f == "eight_clusters") fam = CloudFamily::kEightClusters;
      else if (f == "scanline_single_pass") fam = CloudFamily::kScanlineSinglePass;
      else if (f == "scanline_overlap_multiecho") fam = CloudFamily::kScanlineOverlapMultiecho;
      else { std::fprintf(stderr, "famille inconnue\n"); return 2; } }
    else if (a.rfind("--n=", 0) == 0) n = std::atoi(a.c_str() + 4);
    else if (a.rfind("--s=", 0) == 0) s = std::atoll(a.c_str() + 4);
    else if (a.rfind("--seed=", 0) == 0) seed = std::atoll(a.c_str() + 7);
    else if (a == "--split=level") level_split = true;
  }
  const int coord = cloud_family_default_coord(fam, n);
  const std::vector<P3> pts = make_family_cloud(fam, n, coord, seed);
  const CloudIndex ix = build_cloud_index(pts);
  // buckets sur max(|A|,|B|) : 1, 2-7, 8-63, 64-1023, >=1024
  const char* names[5] = {"max=1", "2-7", "8-63", "64-1023", ">=1024"};
  u64 cnt[5] = {0,0,0,0,0}; u128 mass[5] = {0,0,0,0,0}; u128 sum_factors = 0; u64 maxf = 0, rects = 0;
  u64 both_ge8 = 0, both_ge64 = 0;
  std::vector<WspdRect> wave, next;
  for (const RadixNode& nd : ix.nodes) wave.push_back(WspdRect{nd.left, nd.right});
  while (!wave.empty()) {
    next.clear();
    for (const WspdRect& r : wave) {
      i64 ba[3], bb[3];
      const auto va = detail::node_view(ix, r.a, ba);
      const auto vb = detail::node_view(ix, r.b, bb);
      const u64 wa = detail::node_weight(ix, r.a);
      const u64 wb = detail::node_weight(ix, r.b);
      if (detail::separated(va, vb, s, 1)) {
        ++rects; sum_factors += (u128)(wa + wb);
        const u64 m = wa > wb ? wa : wb; const u64 mn = wa < wb ? wa : wb;
        if (m > maxf) maxf = m;
        int b = m == 1 ? 0 : m < 8 ? 1 : m < 64 ? 2 : m < 1024 ? 3 : 4;
        ++cnt[b]; mass[b] += (u128)wa * wb;
        if (mn >= 8) ++both_ge8;
        if (mn >= 64) ++both_ge64;
        continue;
      }
      const i64 w2a = detail::box_w2(va), w2b = detail::box_w2(vb);
      bool split_a;
      if (!level_split) {
        split_a = (r.a >= 0) && (r.b < 0 || w2a >= w2b);
      } else {
        // cellule de Morton la plus grande = prefixe commun le plus court
        const int pa = r.a >= 0 ? detail::key_delta(ix.keys, ix.nodes[(size_t)r.a].first, ix.nodes[(size_t)r.a].last) : 64;
        const int pb = r.b >= 0 ? detail::key_delta(ix.keys, ix.nodes[(size_t)r.b].first, ix.nodes[(size_t)r.b].last) : 64;
        split_a = (r.a >= 0) && (r.b < 0 || pa <= pb);
      }
      const NodeRef keep = split_a ? r.b : r.a;
      const RadixNode& nd = ix.nodes[(size_t)(split_a ? r.a : r.b)];
      next.push_back(split_a ? WspdRect{nd.left, keep} : WspdRect{keep, nd.left});
      next.push_back(split_a ? WspdRect{nd.right, keep} : WspdRect{keep, nd.right});
    }
    wave.swap(next);
  }
  u128 total_mass = 0; for (int b = 0; b < 5; ++b) total_mass += mass[b];
  // Ledger de masse attendu (comme le probe v4) : C(n,2) - somme_u C(mult_u,2).
  u128 expected = (u128)pts.size() * (pts.size() - 1) / 2;
  for (int u = 0; u < ix.unique_count(); ++u) {
    const u64 w = ix.range_weight(u, u);
    expected -= (u128)w * (w - 1) / 2;
  }
  std::printf("split=%s famille=%s n=%d s=%lld seed=%lld uniques=%d rectangles=%llu sum_factors=%llu max_factor=%llu both_ge8=%llu both_ge64=%llu masse=%llu masse_attendue=%llu\n",
    level_split ? "level" : "diam", cloud_family_name(fam), n, (long long)s, seed, ix.unique_count(), (unsigned long long)rects,
    (unsigned long long)sum_factors, (unsigned long long)maxf, (unsigned long long)both_ge8, (unsigned long long)both_ge64,
    (unsigned long long)total_mass, (unsigned long long)expected);
  for (int b = 0; b < 5; ++b)
    std::printf("  %-8s rect=%10llu (%6.2f%%)  masse_paires=%llu (%6.2f%%)\n", names[b], (unsigned long long)cnt[b],
      100.0 * (double)cnt[b] / (double)rects, (unsigned long long)mass[b], 100.0 * (double)mass[b] / (double)total_mass);
  return total_mass == expected ? 0 : 3;
}
