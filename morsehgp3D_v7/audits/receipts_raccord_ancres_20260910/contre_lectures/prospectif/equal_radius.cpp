#define main recorded_gate_entry
#include "/workspaces/E-HGP/morsehgp3D_v7/tests/full_ball_tower_gate.cpp"
#undef main

// B_sq : centre (20,20,30), R^2=25, coquille 4 points cocirculaires sans paire
// diametrale, deux triangles (ABC, ABD) contenant le centre ; interieurs Q1..Q3
// a z=31 ; B_lot : centre (20,20,18), R^2=169, coquille A,B,C,D + W=(20,20,5).
static std::vector<P3> build(int sym) {
  // offsets dans le plan : A=(5,0) B=(-3,4) C=(-4,-3) D=(0,-5)
  std::vector<std::pair<int,int>> off{{5,0},{-3,4},{-4,-3},{0,-5}};
  std::vector<P3> pts;
  for (auto [a,b] : off) {
    int x=a,y=b;
    if (sym & 1) x=-x;
    if (sym & 2) y=-y;
    if (sym & 4) std::swap(x,y);
    pts.push_back({20+x,20+y,30});
  }
  pts.push_back({20,20,5});
  std::vector<std::pair<int,int>> q{{1,0},{0,1},{-1,0}};
  for (auto [a,b] : q) {
    int x=a,y=b;
    if (sym&1) x=-x;
    if (sym&2) y=-y;
    if (sym&4) std::swap(x,y);
    pts.push_back({20+x,20+y,31});
  }
  return pts;
}

int main() {
  int found = 0;
  for (int sym = 0; sym < 8; ++sym) {
    auto pts = build(sym);
    Fixture fx{"equal_radius8", pts, 8};
    const u64 sr0 = same_radius, iq0 = intruders;
    try {
      for (unsigned variant = 0; variant < 2; ++variant) check_fixture(fx, variant);
      std::printf("sym=%d status=PASS same_radius_steps=%llu intruder_queries=%llu points=", sym,
        (unsigned long long)(same_radius - sr0), (unsigned long long)(intruders - iq0));
      for (auto& p : pts) std::printf("(%lld,%lld,%lld)", (long long)p.x,(long long)p.y,(long long)p.z);
      std::printf("\n");
      if (same_radius - sr0 > 0) ++found;
    } catch (const Failure& f) {
      std::printf("sym=%d status=FAIL why=%s context=%s\n", sym, f.why, context.c_str());
    } catch (const std::exception& e) {
      std::printf("sym=%d status=EXCEPTION what=%s\n", sym, e.what());
    }
  }
  // Ambiguite de fenetre : catalogue construit pour kmax=8, tour demandee a kmax=4.
  {
    auto pts = build(0);
    const auto in = input(Fixture{"w", pts, 8}, 0);
    const auto ix = build_cloud_index(in);
    const oracle::Model model(pts);
    const auto balls8 = catalogue(pts, ix, model, 8);
    const auto balls4 = catalogue(pts, ix, model, 4);
    const auto r84 = build_full_ball_tower(ix, balls8, 4);
    const auto r44 = build_full_ball_tower(ix, balls4, 4);
    std::printf("window: catalogue_k8=%zu balls, catalogue_k4=%zu balls; tower(k8 catalogue, kmax=4) status=%d reason=%s; tower(k4 catalogue, kmax=4) status=%d reason=%s\n",
      balls8.size(), balls4.size(), (int)r84.status, r84.reason, (int)r44.status, r44.reason);
  }
  std::printf("equal_radius_fixtures_found=%d\n", found);
  return found ? 0 : 1;
}
