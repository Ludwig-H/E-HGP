#pragma once
// Bounded oracle and input-plan extensions only, never a product generator.
#include "t2_oracle.hpp"
#include <memory>

namespace export_terminal {
using namespace mhgp7;
inline std::vector<Fixture> planned_fixtures() {
  auto result = fixtures();
  Fixture line{"line12_K9_K10", {}, 10};
  for (i64 j = 0; j < 12; ++j) line.points.push_back({2*j+(j%2),7,9});
  result.push_back(line);
  result.push_back({"shell14_K9_K10_cyclic", {{15,10,10},{5,10,10},{10,15,10},{10,5,10},
      {10,10,15},{10,10,5},{13,14,10},{13,6,10},{7,14,10},{7,6,10},{10,13,14},{10,7,6},
      {10,10,10},{20,25,30}},10});
  result.push_back({"spatial12_K9_K10", {{7,42,83},{91,12,64},{33,88,9},{54,20,71},
      {18,61,39},{76,53,95},{42,7,24},{62,94,47},{3,29,58},{85,73,15},{29,36,97},{58,65,3}},10});
  return result;
}
// Masks are slots of the deterministic Morton-sorted unique cloud. Selection
// uses only n/K and these declared cyclic shapes, never a computed MEB/status.
inline std::vector<u32> declared_masks(const Fixture& fixture) {
  const u32 n = static_cast<u32>(fixture.points.size());
  std::vector<u32> result;
  if (n <= 8 || n == 12) {
    for (u32 mask = 1; mask < (u32{1} << n); ++mask) {
      const auto k = std::popcount(mask);
      if ((n <= 8 && k >= 2) || (n == 12 && (k == 9 || k == 10))) result.push_back(mask);
    }
  } else {
    need(n == 14,"plan.fixture_domain");
    for (u32 k : {9u,10u}) for (u32 rotation = 0; rotation < n; ++rotation) {
      u32 contiguous = 0, skipped = 0;
      for (u32 i = 0; i < k; ++i) contiguous |= u32{1} << ((rotation+i)%n);
      for (u32 i = 0; i + 1 < k; ++i) skipped |= u32{1} << ((rotation+i)%n);
      skipped |= u32{1} << ((rotation+k)%n);
      result.push_back(contiguous); result.push_back(skipped);
    }
    std::sort(result.begin(),result.end());
    result.erase(std::unique(result.begin(),result.end()),result.end());
  }
  need(result.size() == (n == 12 ? 286u : (n == 14 ? 56u : (u32{1} << n)-1-n)),
      "plan.predeclared_count");
  return result;
}

// Same independent q<=4 Gram catalogue construction as T2, with its census
// window applied BEFORE writing fixed-size BallData interior/shell arrays.
inline std::vector<BallData> large_catalogue(const std::vector<P3>& points,const CloudIndex& ix) {
  struct Row { oracle::Ball ball; unsigned qmin; };
  std::map<BallKey,Row> candidates;
  for (u32 support = 1; support < (u32{1} << points.size()); ++support) {
    const unsigned q = std::popcount(support);
    if (q < 2 || q > 4) continue;
    const auto ball = oracle::detail::support_ball(points,support);
    if (!ball) continue;
    const auto [found,inserted] = candidates.emplace(key(*ball),Row{*ball,q});
    if (!inserted) found->second.qmin = std::min(found->second.qmin,q);
  }
  std::vector<BallData> result;
  for (const auto& [ball_key,row] : candidates) {
    std::vector<i32> interior,shell;
    for (size_t i = 0; i < ix.upos.size(); ++i) {
      const auto d = distance2(ix.upos[i],row.ball);
      if (d < row.ball.radius2) interior.push_back(static_cast<i32>(i));
      else if (d == row.ball.radius2) shell.push_back(static_cast<i32>(i));
    }
    if (interior.size()+row.qmin > 11) continue;  // declared Kmax10 census window
    need(interior.size() <= kBallInteriorMax && shell.size() <= kBallShellMax,
        "large.catalogue_representation");
    BallData ball;
    ball.key = ball_key; ball.level = level(row.ball.radius2); ball.arity = static_cast<u8>(row.qmin);
    ball.n_interior = static_cast<u8>(interior.size()); ball.n_shell = static_cast<u8>(shell.size());
    std::copy(interior.begin(),interior.end(),ball.interior_ids);
    std::copy(shell.begin(),shell.end(),ball.shell_ids);
    result.push_back(ball);
  }
  return result;
}
struct FixtureOracle {
  std::unique_ptr<oracle::Model> historical;
  std::unique_ptr<t2_plateau_oracle::Model> assigned;
  explicit FixtureOracle(const Fixture& fixture) {
    assigned = std::make_unique<t2_plateau_oracle::Model>(fixture.points);
    if (fixture.points.size() <= 8) historical = std::make_unique<oracle::Model>(fixture.points);
  }
  oracle::Ball meb(u32 mask) const { return historical ? historical->meb(mask) : assigned->meb(mask); }
};
inline int print_plan() {
  std::printf("{\"status\":\"declared\",\"mask_space\":\"Morton_geometric_indices\",\"clouds\":[");
  bool first = true;
  u64 counts[11]{};
  for (const auto& fixture : planned_fixtures()) {
    const auto masks = declared_masks(fixture);
    if (!first) std::printf(",");
    first = false;
    std::printf("{\"name\":\"%s\",\"n\":%zu,\"masks\":[",fixture.name,fixture.points.size());
    for (size_t i = 0; i < masks.size(); ++i) {
      if (i) std::printf(",");
      std::printf("%u",masks[i]); ++counts[std::popcount(masks[i])];
    }
    std::printf("]}");
  }
  need(counts[9] == 468 && counts[10] == 160,"plan.K9_K10_count");
  std::printf("],\"K9_requests\":%llu,\"K10_requests\":%llu,\"geometry_executed\":false}\n",
      (unsigned long long)counts[9],(unsigned long long)counts[10]);
  return 0;
}
}  // namespace export_terminal
