// MorseHGP3D v5 — porte du COVER D'ARETE par handles (lanes/edge_cover.hpp).
// Pour chaque rectangle terminal de la WSPD (uniform n=400) et chaque ancre
// (a,b) du rectangle, le cover par handles (`rect_cover_handles` sur le
// rectangle puis `anchor_cover_from_handles`) doit rendre EXACTEMENT le meme
// ensemble de sites que la requete directe `cover_query` (coefficient 3), et
// un ensemble non vide pour au moins --min-anchors ancres. Mutant
// `cover-rect-dmin` (borne du rectangle par Dmin au lieu de Dmax) : des sites
// manquent -> code 4. Codes : 0, 3 (desaccord/plancher), 4.
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

#include "../src/cloud/families.hpp"
#include "../src/lanes/edge_cover.hpp"
#include "../src/wspd/wavefront.hpp"

using namespace mhgp5;

int main(int argc, char** argv) {
  std::string inject;
  int n = 400;
  u64 min_anchors = 1000;
  for (int i = 1; i < argc; ++i) {
    const std::string arg = argv[i];
    if (arg.rfind("--inject=", 0) == 0) inject = arg.substr(9);
    else if (arg.rfind("--n=", 0) == 0) n = std::atoi(arg.c_str() + 4);
    else if (arg.rfind("--min-anchors=", 0) == 0) min_anchors = (u64)std::atoll(arg.c_str() + 14);
    else return 2;
  }
  if (!inject.empty() && !mutants_enable(inject)) return 2;
  const CloudIndex ix = build_cloud_index(make_family_input(CloudFamily::kUniform, n, 0, 3));
  if (!ix.valid) return 2;
  std::vector<WspdRect> rects;
  wspd_wavefront(ix, 8, 1, [&](const WspdRect& r) { rects.push_back(r); });
  u64 anchors = 0, mismatches = 0, nonempty = 0;
  std::vector<NodeRef> handles;
  std::vector<CoverPoint> by_handles, direct, scratch;
  u64 nodes = 0, visits = 0;
  for (const WspdRect& r : rects) {
    rect_cover_handles(ix, ix.box_of(r.a), ix.box_of(r.b), 3, &handles, &nodes);
    const NodeRange ra = ix.range_of(r.a), rb = ix.range_of(r.b);
    for (i32 ua = ra.first; ua <= ra.last; ++ua)
      for (i32 ub = rb.first; ub <= rb.last; ++ub) {
        const P3& pa = ix.upos[(size_t)ua];
        const P3& pb = ix.upos[(size_t)ub];
        const i64 D2 = p3_norm2(p3_sub(pb, pa));
        if (D2 == 0) continue;
        ++anchors;
        anchor_cover_from_handles(ix, handles, pa, pb, D2, 3, &by_handles, &visits, &scratch);
        cover_query(ix, pa, pb, D2, 3, &direct);
        std::vector<i32> x, y;
        for (const CoverPoint& c : by_handles) x.push_back(c.u);
        for (const CoverPoint& c : direct) y.push_back(c.u);
        std::sort(x.begin(), x.end());
        std::sort(y.begin(), y.end());
        if (x != y) ++mismatches;
        if (!y.empty()) ++nonempty;
      }
  }
  std::printf("cover_gate rectangles=%zu ancres=%llu non_vides=%llu desaccords=%llu\n", rects.size(),
              (unsigned long long)anchors, (unsigned long long)nonempty, (unsigned long long)mismatches);
  if (!inject.empty()) {
    if (mismatches) { std::fprintf(stderr, "MUTANT TUE : %s\n", inject.c_str()); return 4; }
    std::fprintf(stderr, "PORTE INEFFICACE : mutant %s survivant\n", inject.c_str());
    return 3;
  }
  if (mismatches || nonempty < min_anchors) return 3;
  std::printf("cover_gate OK\n");
  return 0;
}
