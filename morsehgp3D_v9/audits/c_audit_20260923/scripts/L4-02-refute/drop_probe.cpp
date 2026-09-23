// Bounded independent Gram/Gamma judge of the ball-catalogue FULL producer.
// All subset enumeration, explicit facet membership and point sets live HERE,
// never in the product. Catalogues are not made by WSPD or product predicates.
#include <algorithm>
#include <bit>
#include <cstdio>
#include <limits>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "/workspaces/E-HGP/build/v9-audit-c-worktree/morsehgp3D_v9/oracle/tower/local_plateau_oracle.hpp"
#include "/workspaces/E-HGP/build/v9-audit-c-worktree/morsehgp3D_v9/src/tower/forest/full_ball_tower.hpp"

#include <random>
namespace {
using namespace mhgp9::tower;
namespace oracle = local_plateau_oracle;
using oracle::Int;
using oracle::Rat;
struct Failure { const char* why; };
void need(bool good, const char* why) { if (!good) throw Failure{why}; }
Int integer(i128 n) {
  const bool negative = n < 0;
  const u128 magnitude = negative ? static_cast<u128>(-(n + 1)) + 1 : static_cast<u128>(n);
  Int value = static_cast<u64>(magnitude >> 64);
  value <<= 64;
  value += static_cast<u64>(magnitude);
  return negative ? -value : value;
}

i128 narrow(const Int& value) {
  const bool negative = value < 0;
  const Int magnitude = negative ? -value : value;
  need(magnitude < (Int(1) << 127), "oracle.int128_fixture_bound");
  const Int low = magnitude & ((Int(1) << 64) - 1);
  const Int high = magnitude >> 64;
  const u128 packed = (static_cast<u128>(high.convert_to<u64>()) << 64) |
      low.convert_to<u64>();
  return negative ? -static_cast<i128>(packed) : static_cast<i128>(packed);
}

Int gcd(Int a, Int b) {
  if (a < 0) a = -a;
  if (b < 0) b = -b;
  while (b != 0) { const Int remainder = a % b; a = b; b = remainder; }
  return a;
}

BallKey key(const oracle::Ball& ball) {
  std::array<Rat, 5> coefficients{Rat(1), -Rat(2) * ball.center[0],
      -Rat(2) * ball.center[1], -Rat(2) * ball.center[2], -ball.radius2};
  for (const auto& coordinate : ball.center) coefficients[4] += coordinate * coordinate;
  Int denominator = 1;
  for (const auto& c : coefficients)
    denominator = denominator / gcd(denominator, c.denominator()) * c.denominator();
  std::array<Int, 5> integral;
  Int divisor = 0;
  for (size_t j = 0; j < coefficients.size(); ++j) {
    integral[j] = coefficients[j].numerator() * (denominator / coefficients[j].denominator());
    divisor = gcd(divisor, integral[j]);
  }
  for (auto& c : integral) c /= divisor;
  return {narrow(integral[0]), {narrow(integral[1]), narrow(integral[2]),
      narrow(integral[3])}, narrow(integral[4])};
}

ExactLevel level(const Rat& value) {
  need(value >= Rat(0) && value.numerator() < (Int(1) << 192), "oracle.level_bound");
  ExactLevel result{};
  Int remaining = value.numerator();
  const Int mask = (Int(1) << 64) - 1;
  for (size_t j = 0; j < 3; ++j) {
    result.num[j] = (remaining & mask).convert_to<u64>();
    remaining >>= 64;
  }
  result.den = narrow(value.denominator());
  return result;
}

Rat rational(const ExactLevel& value) {
  Int numerator = 0;
  for (size_t j = 3; j-- > 0;) { numerator <<= 64; numerator += value.num[j]; }
  need(value.den > 0, "output.positive_denominator");
  return Rat(numerator, integer(value.den));
}

Rat distance2(const P3& p, const oracle::Ball& ball) {
  const std::array<i64, 3> coordinates{p.x, p.y, p.z};
  Rat result(0);
  for (size_t j = 0; j < 3; ++j) {
    const Rat delta = Rat(coordinates[j]) - ball.center[j];
    result += delta * delta;
  }
  return result;
}
std::vector<BallData> catalogue(const std::vector<P3>& points, const CloudIndex& ix,
    const oracle::Model& model, unsigned kmax) {
  std::map<BallKey, oracle::Ball> balls;
  const u32 domain = (u32{1} << points.size()) - 1;
  for (u32 mask = 1; mask <= domain; ++mask) {
    const auto ball = model.meb(mask);
    if (ball.radius2 != Rat(0)) balls.emplace(key(ball), ball);
  }
  std::vector<BallData> result;
  for (const auto& [ball_key, ball] : balls) {
    BallData row{};
    row.key = ball_key;
    row.level = level(ball.radius2);
    u32 shell = 0;
    for (size_t bit = 0; bit < points.size(); ++bit) {
      const Rat d = distance2(points[bit], ball);
      if (d > ball.radius2) continue;
      const auto found = std::find(ix.upos.begin(), ix.upos.end(), points[bit]);
      need(found != ix.upos.end(), "oracle.geometry_index");
      const i32 index = static_cast<i32>(found - ix.upos.begin());
      if (d == ball.radius2) {
        need(row.n_shell < kBallShellMax, "oracle.shell_fixture_bound");
        row.shell_ids[row.n_shell++] = index;
        shell |= u32{1} << bit;
      } else {
        need(row.n_interior < kBallInteriorMax, "oracle.interior_fixture_bound");
        row.interior_ids[row.n_interior++] = index;
      }
    }
    unsigned qmin = 5;
    for (u32 support = shell; support; support = (support - 1) & shell) {
      const unsigned q = std::popcount(support);
      if (q >= qmin || q > 4) continue;
      const auto candidate = oracle::detail::support_ball(points, support);
      if (candidate && candidate->center == ball.center && candidate->radius2 == ball.radius2)
        qmin = q;
    }
    need(qmin >= 2 && qmin <= 4, "oracle.positive_minimal_support");
    row.arity = static_cast<u8>(qmin);
    if (row.n_interior + qmin > std::min<size_t>(kmax + 1, points.size())) continue;
    std::sort(row.interior_ids, row.interior_ids + row.n_interior);
    std::sort(row.shell_ids, row.shell_ids + row.n_shell);
    result.push_back(row);
  }
  return result;
}

}  // namespace

int main() {
  std::mt19937 rng(20260923);
  std::vector<std::pair<const char*, std::vector<P3>>> clouds = {
    {"shell7_window", {{10,5,0},{0,5,0},{5,10,0},{5,0,0},{8,9,0},{2,1,0},{9,8,0}}},
    {"portal_equal", {{0,2,0},{2,4,0},{4,2,0},{2,0,0},{2,2,0},{2,1,0}}},
    {"grid8", {{0,0,0},{1,0,0},{0,1,0},{1,1,0},{0,0,1},{1,0,1},{0,1,1},{2,1,1}}},
  };
  for (int c = 0; c < 6; ++c) {
    std::uniform_int_distribution<int> d(0, 1000);
    std::vector<P3> pts;
    for (int j = 0; j < 8; ++j) pts.push_back({d(rng), d(rng), d(rng)});
    clouds.push_back({"random8", pts});
  }
  // counters[top<=kmax][detected]
  unsigned long long tally[2][2] = {{0,0},{0,0}}, k1_mst_undetected = 0, k1_drops = 0, k1_detected_by_k2 = 0;
  unsigned long long plateau_hi_detected = 0;
  for (auto& [name, pts] : clouds) {
    std::vector<InputPoint> in;
    for (size_t j = 0; j < pts.size(); ++j) in.push_back({static_cast<PointId>(j * 7 + 3), pts[j]});
    const auto ix = build_cloud_index(in);
    const oracle::Model model(pts);
    for (unsigned kmax = 1; kmax < pts.size(); ++kmax) {
      const auto balls = catalogue(pts, ix, model, kmax);
      for (int threads : {0, 2}) {
        const auto full = build_full_ball_tower(ix, balls, kmax, threads);
        if (full.status != FullBallStatus::kCompleteRelative) { std::printf("BASE FAIL %s k=%u %s\n", name, kmax, full.reason); return 2; }
        for (size_t drop = 0; drop < balls.size(); ++drop) {
          auto cat = balls; cat.erase(cat.begin() + drop);
          const auto& b = balls[drop];
          const unsigned top = b.n_interior + b.n_shell;
          const auto r = build_full_ball_tower(ix, cat, kmax, threads);
          const bool detected = r.status != FullBallStatus::kCompleteRelative;
          tally[top <= kmax ? 0 : 1][detected]++;
          if (top <= kmax && !detected)
            std::printf("UNDETECTED-LOW %s kmax=%u thr=%d p=%u q=%u u=%u\n", name, kmax, threads, b.n_interior, b.arity, b.n_shell);
          if (top > kmax && detected) { ++plateau_hi_detected;
            std::printf("DETECTED-HIGH %s kmax=%u thr=%d p=%u q=%u u=%u reason=%s\n", name, kmax, threads, b.n_interior, b.arity, b.n_shell, r.reason); }
          if (b.n_interior == 0 && b.arity == 2 && threads == 0) {
            if (kmax == 1) { ++k1_drops; if (!detected) ++k1_mst_undetected; }
            if (kmax == 2 && b.n_shell == 2 && detected) ++k1_detected_by_k2;
          }
          if (threads == 0 && kmax <= 2 && detected) std::printf("  reason %s kmax=%u p=%u q=%u u=%u: %s\n", name, kmax, b.n_interior, b.arity, b.n_shell, r.reason);
        }
      }
    }
  }
  std::printf("top<=kmax: detected=%llu undetected=%llu\n", tally[0][1], tally[0][0]);
  std::printf("top>kmax : detected=%llu undetected=%llu\n", tally[1][1], tally[1][0]);
  std::printf("K1 Gabriel drops at kmax=1: %llu, undetected=%llu ; regular Gabriel drops detected at kmax=2: %llu\n", k1_drops, k1_mst_undetected, k1_detected_by_k2);
  return 0;
}
