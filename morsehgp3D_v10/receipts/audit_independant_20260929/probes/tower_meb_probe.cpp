// Sonde des fonctions internes ; inclure UNIQUEMENT la source figee indiquee a la compilation.
#include <iostream>
#include <vector>
#include "tower/tower.cpp"

int main() {
  using namespace mhgp10;
  u32 n;
  while (std::cin >> n) {
    std::vector<u32> x(n), y(n), z(n), pid(n);
    for (u32 i = 0; i < n; ++i) { std::cin >> x[i] >> y[i] >> z[i]; pid[i] = i; }
    auto cloud = prepare_cloud(x, y, z, pid, kCoordinateBits);
    if (!cloud.ok() || n > kMaxFacet) return 2;
    SiteTree tree(cloud.value());
    Catalogue cat;
    Geo g{cloud.value(), tree, cat, {}, {}};
    for (u32 s = 0; s < cloud.value().sites(); ++s)
      g.P.push_back(P3{i64(cloud.value().x[s]), i64(cloud.value().y[s]), i64(cloud.value().z[s])});
    Facet F;
    F.n = n;
    for (u32 i = 0; i < n; ++i) {
      const P3 p{i64(x[i]), i64(y[i]), i64(z[i])};
      for (u32 s = 0; s < n; ++s) if (g.P[s].x == p.x && g.P[s].y == p.y && g.P[s].z == p.z) F.s[i] = s;
    }
    u32 R[4];
    Sphere W = welzl(g, F.s.data(), int(n), R, 0);
    W.has_level = true;
    u64 fallbacks = 0;
    Sphere M = meb(g, F, fallbacks);
    for (Sphere S : {W, M}) {
      std::cout << (S.zero ? "0" : arith::to_string(S.has_level ? S.level.num : level_of(g, S).num)) << ' '
                << (S.zero ? "1" : arith::to_string(S.has_level ? S.level.den : level_of(g, S).den)) << ' ';
      if (S.empty) { std::cout << "empty\n"; return 3; }
      const auto a = g.P[S.anchor];
      auto str128 = [](i128 v) { return arith::to_string(arith::I128w::from_i128(v)); };
      std::cout << a.x << ' ' << a.y << ' ' << a.z << ' ' << str128(S.c.N[0]) << ' '
                << str128(S.c.N[1]) << ' ' << str128(S.c.N[2]) << ' ' << str128(S.c.D) << ' ';
    }
    std::cout << fallbacks << '\n';
  }
}
