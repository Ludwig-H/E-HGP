// Audit-only adapter. The independent rational judge is in the companion Python.
#include <array>
#include <iostream>
#include <string>
#include <vector>
#include "src/forest/silent_incidence.hpp"

using namespace mhgp7;

std::string decimal(i128 value) {
  const bool negative = value < 0;
  u128 n = negative ? static_cast<u128>(-(value + 1)) + 1 : static_cast<u128>(value);
  std::string out;
  do { out.push_back(static_cast<char>('0' + n % 10)); n /= 10; } while (n);
  if (negative) out.push_back('-');
  std::reverse(out.begin(), out.end());
  return out;
}

void print_key(const BallKey& key) {
  std::cout << '[' << decimal(key.a);
  for (const i128 value : key.b) std::cout << ',' << decimal(value);
  std::cout << ',' << decimal(key.c) << ']';
}

int main(int argc, char** argv) {
  if (argc == 2) {
    const std::string arg(argv[1]);
    if (!arg.starts_with("--mutant=") || !mutants_enable(arg.substr(9))) return 2;
  } else if (argc != 1) return 2;
  char mode = 0;
  while (std::cin >> mode) {
    if (mode == 'M') {
      size_t n = 0;
      u64 cap = 0;
      if (!(std::cin >> n >> cap) || n < 2 || n > 11) return 2;
      std::vector<P3> points(n);
      for (auto& p : points) if (!(std::cin >> p.x >> p.y >> p.z)) return 2;
      const CloudIndex ix = build_cloud_index(points);
      if (!ix.valid || ix.has_duplicate_positions()) return 2;
      std::array<i32, 11> sites{};
      for (i32 u = 0; u < ix.unique_count(); ++u) sites[ix.point_id(u)] = u;
      SilentIncidenceLimits caps;
      caps.max_meb_supports = cap;
      SilentIncidenceResult result;
      const std::vector<ForestEvent> direct;
      silent_detail::Builder builder(ix, direct, caps, &result);
      silent_detail::LocalBall ball;
      ball.key = {7, {11, 13, 17}, 19};
      ball.level = {{23, 29, 31}, 37};
      ball.q = 9;
      ball.support = {-1, -2, -3, -4};
      const bool ok = builder.miniball(sites, n, &ball);
      std::cout << "{\"ok\":" << (ok ? "true" : "false")
                << ",\"status\":" << static_cast<int>(result.status)
                << ",\"reason\":\"" << result.reason << "\",\"supports\":"
                << result.stats.meb_supports << ",\"calls\":" << result.stats.meb_calls
                << ",\"q\":" << static_cast<unsigned>(ball.q) << ",\"key\":";
      print_key(ball.key);
      std::cout << ",\"num\":[" << ball.level.num[0] << ',' << ball.level.num[1]
                << ',' << ball.level.num[2] << "],\"den\":" << decimal(ball.level.den)
                << ",\"support\":[";
      for (size_t i = 0; i < 4; ++i) {
        if (i) std::cout << ',';
        std::cout << (ball.support[i] < 0 ? ball.support[i] :
                     static_cast<i64>(ix.point_id(ball.support[i])));
      }
      std::cout << "]}\n";
    } else if (mode == 'P') {
      size_t q = 0;
      if (!(std::cin >> q) || (q != 3 && q != 4)) return 2;
      std::array<P3, 4> points{};
      P3 z;
      for (size_t i = 0; i < q; ++i)
        if (!(std::cin >> points[i].x >> points[i].y >> points[i].z)) return 2;
      if (!(std::cin >> z.x >> z.y >> z.z)) return 2;
      BallKey key{};
      i128 raw = 0, scale = 0;
      if (q == 3) {
        const auto form = q3_form(points[0], points[1], points[2]);
        if (form.g <= 0) return 2;
        scale = form.g;
        raw = q3_power(form, z);
        key = q3_ball_key(form);
      } else {
        const auto form = q4_form(points[0], points[1], points[2], points[3]);
        if (form.det <= 0) return 2;
        scale = form.det;
        raw = q4_power(form, z);
        key = ball_key_reduce(q4_ball_form(form));
      }
      std::cout << "{\"raw\":" << decimal(raw) << ",\"primitive\":"
                << decimal(key.power(z)) << ",\"scale\":" << decimal(scale)
                << ",\"key\":";
      print_key(key);
      std::cout << "}\n";
    } else return 2;
  }
  return std::cin.eof() ? 0 : 2;
}
