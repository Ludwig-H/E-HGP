// Sonde bornee de l'audit independant ; aucun changement du code produit.
#include <algorithm>
#include <cstdio>
#include <vector>
#include "arith/geometry.hpp"
#include "cloud/site_tree.hpp"

using namespace mhgp10;

int main() {
  const std::vector<u32> x{0, 225077, 225068, 152369};
  const std::vector<u32> y{0, 1, 1, 7}, z{0, 0, 0, 1}, ids{0, 1, 2, 3};
  auto made = prepare_cloud(x, y, z, ids, 18);
  if (!made.ok()) return 3;
  const Cloud& cloud = made.value();
  SiteTree tree(cloud);
  const geom::P3 anchor{0, 0, 0}, p1{225077, 1, 0}, p2{225068, 1, 0}, p3{152369, 7, 1};
  geom::Center ctr;
  if (!geom::center4(anchor, p1, p2, p3, ctr)) return 3;
  const geom::P3* tetra[4] = {&anchor, &p1, &p2, &p3};
  std::vector<u32> interior, shell, want;
  for (u32 s = 0; s < cloud.sites(); ++s) {
    const geom::P3 p{i64(cloud.x[s]), i64(cloud.y[s]), i64(cloud.z[s])};
    if (geom::side_key(ctr, anchor, p) == 0) want.push_back(s);
  }
  tree.closed_ball(anchor, ctr, interior, shell);
  const double cx = double(anchor.x) + double(ctr.N[0]) / double(ctr.D);
  const double cy = double(anchor.y) + double(ctr.N[1]) / double(ctr.D);
  const double cz = double(anchor.z) + double(ctr.N[2]) / double(ctr.D);
  std::printf("center=(%.17g,%.17g,%.17g) strictly_inside=%d expected_shell=%zu got_shell=%zu got_interior=%zu\n",
              cx, cy, cz, int(geom::strictly_inside_tetra(tetra, anchor, ctr)), want.size(), shell.size(), interior.size());
  for (u32 s = 0; s < cloud.sites(); ++s) {
    const geom::P3 p{i64(cloud.x[s]), i64(cloud.y[s]), i64(cloud.z[s])};
    const double dx = double(p.x)-cx, dy=double(p.y)-cy, dz=double(p.z)-cz;
    const double approx = dx*dx+dy*dy+dz*dz;
    const double r2a = cx*cx+cy*cy+cz*cz;
    std::printf("site=%u point=(%lld,%lld,%lld) exact_side=%d approx_delta=%.17g in_shell=%d in_interior=%d\n", s,
                static_cast<long long>(p.x), static_cast<long long>(p.y), static_cast<long long>(p.z),
                geom::side(ctr, anchor, p), approx-r2a, int(std::find(shell.begin(),shell.end(),s)!=shell.end()),
                int(std::find(interior.begin(),interior.end(),s)!=interior.end()));
  }
  for (u32 count : {1u,3u,4u}) {
    std::vector<std::pair<i128,u32>> nearest;
    tree.nearest(anchor,ctr,count,nearest);
    bool exact = nearest.size() == count;
    for (u32 i=0; exact && i<count; ++i) exact = nearest[i].first == 0 && nearest[i].second == i;
    std::printf("nearest_count=%u got=%zu exact=%d\n",count,nearest.size(),int(exact));
  }
  return shell == want && interior.empty() ? 0 : 1;
}
