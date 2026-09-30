#include <cfenv>
#include <limits>
#include <cstdio>

#define main site_tree_regression_main
#include "/workspaces/E-HGP/build/v10-fixes/sitetree/src/morsehgp3D_v10/tests/regression/site_tree_far_center.cpp"
#undef main

// New independent boundary checks: analytic collinear cloud, query caps,
// and only the promised total filtered() guard for forged numeric inputs.
bool boundaries() {
  std::vector<geom::P3> pts;
  for (i64 x = 0; x < 128; ++x) pts.push_back({x, 0, 0});
  const Cloud cloud = make_cloud(pts, 18);
  const SiteTree tree(cloud);
  const geom::P3 a{0, 0, 0};
  geom::Center ctr[2];
  geom::center2(a, {1, 0, 0}, ctr[0]);
  if (!geom::center3(a, {0, 4, 0}, {1, 2, 0}, ctr[1])) return false;
  bool ok = tree.filtered(a, ctr[0]) && !tree.filtered(a, ctr[1]);
  unsigned checks = 0;
  for (unsigned c = 0; c < 2; ++c) {
    std::vector<u32> inside{999}, shell{999};
    tree.closed_ball(a, ctr[c], inside, shell);
    ok &= inside.empty() && shell == (c == 0 ? std::vector<u32>{0, 1} : std::vector<u32>{0});
    ++checks;
    for (u32 count : {0U, 1U, 64U, 65U, 127U, 128U}) {
      std::vector<std::pair<i128, u32>> out{{123, 999}};
      tree.nearest(a, ctr[c], count, out);
      const u32 wanted = std::min(count, 64U);
      ok &= out.size() == wanted;
      for (u32 s = 0; s < out.size(); ++s) {
        // Analytic key from collinear squared distances to 1/2 or (-3/2,2,0).
        const i128 key = c == 0 ? 2 * i128(s) * (i128(s) - 1)
                               : ctr[c].D * i128(s) * (i128(s) + 3);
        ok &= out[s].second == s && out[s].first == key;
      }
      ++checks;
    }
  }
  const i128 vmax = static_cast<i128>((u128{1} << 127) - 1), vmin = -vmax - 1;
  for (i128 d : {vmin, i128{-1}, i128{0}, (i128{1} << 82) + 1, vmax}) {
    const geom::Center bad{{0, 0, 0}, d};
    ok &= !tree.filtered(a, bad);
    ++checks;
  }
  for (i128 n : {vmin, -(i128{1} << 100) - 1, (i128{1} << 100) + 1, vmax}) {
    const geom::Center bad{{n, 0, 0}, 1};
    ok &= !tree.filtered(a, bad);
    ++checks;
  }
  for (i64 x : {std::numeric_limits<i64>::min(), i64{-1}, kL + 1, std::numeric_limits<i64>::max()}) {
    ok &= !tree.filtered({x, 0, 0}, geom::Center{{0, 0, 0}, 1});
    ++checks;
  }
  ok &= tree.filtered(a, geom::Center{{0, 0, 0}, i128{1} << 82});
  ++checks;
  std::printf("independent_boundaries checks=%u ok=%d\n", checks, int(ok));
  return ok;
}

int main() {
  const int saved = std::fegetround();
  const int modes[] = {FE_TONEAREST, FE_UPWARD, FE_DOWNWARD, FE_TOWARDZERO};
  const char* names[] = {"nearest", "upward", "downward", "towardzero"};
  unsigned failed_modes = 0;
  for (unsigned m = 0; m < 4; ++m) {
    if (std::fesetround(modes[m]) != 0) return 3;
    failures = 0;
    std::printf("rounding_begin mode=%s fegetround=%d\n", names[m], std::fegetround());
    const int gate = site_tree_regression_main();
    const bool extra = boundaries(), retained = std::fegetround() == modes[m];
    std::printf("rounding_end mode=%s gate_code=%d boundaries=%d retained=%d\n", names[m], gate, int(extra), int(retained));
    failed_modes += gate != 0 || !extra || !retained;
  }
  std::fesetround(saved);
  std::printf("rounding_summary modes=4 failed=%u\n", failed_modes);
  return failed_modes ? 1 : 0;
}
