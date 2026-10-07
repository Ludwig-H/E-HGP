// Harnais L06 : les fonctions de MEB de tower.cpp sont dans un espace de noms anonyme, donc inatteignables par un
// test du depot. On inclut le fichier source dans la meme unite de traduction pour les appeler :
//   welzl()  : repli exact (Welzl recursif) ;  meb() : chemin produit (proposition double + certificat, sinon repli).
// Entree (stdin) : par ligne « n x0 y0 z0 x1 y1 z1 ... ». Sortie : par ligne « num_w/den_w num_m/den_m repli ».
#include "/workspaces/E-HGP/build/v11-worktree/morsehgp3D_v10/src/tower/tower.cpp"

#include <iostream>
#include <sstream>

int main() {
  using namespace mhgp10;
  Cloud cloud;
  SiteTree tree(cloud);
  Catalogue cat;
  Geo g{cloud, tree, cat, {}, {}};
  std::string line;
  while (std::getline(std::cin, line)) {
    std::istringstream is(line);
    int n;
    is >> n;
    g.P.clear();
    for (int i = 0; i < n; ++i) {
      long long x, y, z;
      is >> x >> y >> z;
      g.P.push_back(geom::P3{x, y, z});
    }
    Facet F;
    F.n = static_cast<u32>(n);
    for (int i = 0; i < n; ++i) F.s[i] = static_cast<u32>(i);
    u32 R[4];
    Sphere W = welzl(g, F.s.data(), n, R, 0);
    u64 fb = 0;
    Sphere M = meb(g, F, fb);
    const geom::Level& lm = level_of(g, M);
    std::cout << arith::to_string(W.level.num) << "/" << arith::to_string(W.level.den) << " " << arith::to_string(lm.num)
              << "/" << arith::to_string(lm.den) << " " << fb << "\n";
  }
  return 0;
}
