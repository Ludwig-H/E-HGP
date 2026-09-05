// Audit adapter to real C++ helpers; the rational judge lives in Python.
#include <array>
#include <cfenv>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include "src/pipeline/generate.hpp"

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

bool read_integer(i128* out) {
  std::string token;
  if (!(std::cin >> token) || token.empty()) return false;
  const bool negative = token[0] == '-';
  u128 value = 0;
  for (size_t i = negative ? 1 : 0; i < token.size(); ++i) {
    if (token[i] < '0' || token[i] > '9') return false;
    value = 10 * value + static_cast<unsigned>(token[i] - '0');
    if (value >= (static_cast<u128>(1) << 121)) return false;
  }
  *out = negative ? -static_cast<i128>(value) : static_cast<i128>(value);
  return true;
}

bool point(P3* p) { return static_cast<bool>(std::cin >> p->x >> p->y >> p->z) && p3_in_profile(*p); }

bool rounding(int mode) {
  const int values[4] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};
  return mode >= 0 && mode < 4 && std::fesetround(values[mode]) == 0;
}

void array3(const i64* values) {
  std::cout << '[' << values[0] << ',' << values[1] << ',' << values[2] << ']';
}

void chord_output(ChordPieces* chord, double lh, double error, i64 b, i128 exact) {
  std::cout << "\"mu\":" << decimal(chord->mu_hat) << ",\"signs\":[";
  for (int j = 0; j <= 4; ++j) {
    if (j) std::cout << ',';
    std::cout << chord->certified_sign(lh, error, 2 * j - 4, b);
  }
  int calls = 0;
  chord->update(lh, error, b, [&]() { ++calls; return exact; });
  std::cout << "],\"exact_calls\":" << calls << ",\"counts\":[";
  for (int i = 0; i < 4; ++i) {
    if (i) std::cout << ',';
    std::cout << chord->cnt[i];
  }
  std::cout << "],\"dead1\":" << (chord->dead(1) ? "true" : "false")
            << ",\"dead2\":" << (chord->dead(2) ? "true" : "false");
}

int main(int argc, char** argv) {
  bool nonstrict_parameter = false;
  if (argc == 2) {
    const std::string arg(argv[1]);
    if (arg == "--fault=chord-nonstrict-parameter") nonstrict_parameter = true;
    else if (arg.starts_with("--mutant=") && mutants_enable(arg.substr(9))) {}
    else return 2;
  } else if (argc != 1) return 2;
  char command = 0;
  while (std::cin >> command) {
    if (command == 'B') {
      P3 a, b;
      i64 den = 0;
      if (!point(&a) || !point(&b) || !(std::cin >> den) || (den != 8 && den != 12) || a == b) return 2;
      i64 u[3]{}, v[3]{};
      const bool ok = bisector_basis(a, b, p3_norm2(p3_sub(b, a)), den, u, v);
      std::cout << "{\"ok\":" << (ok ? "true" : "false") << ",\"u\":";
      array3(u);
      std::cout << ",\"v\":";
      array3(v);
      std::cout << "}\n";
    } else if (command == 'S') {
      int mode = 0;
      i128 value = 0;
      if (!(std::cin >> mode) || !read_integer(&value) || value < 0 || value >= (static_cast<i128>(1) << 120) || !rounding(mode)) return 2;
      const i128 result = isqrt128_floor(value);
      std::cout << "{\"root\":" << decimal(result) << "}\n";
    } else if (command == 'C') {
      int mode = 0, force_exact = 0;
      i128 j = 0, exact = 0;
      i64 b = 0;
      if (!(std::cin >> mode >> force_exact) || !read_integer(&j) || !(std::cin >> b) ||
          !read_integer(&exact) || j < 0 || j >= (static_cast<i128>(1) << 103) || !rounding(mode)) return 2;
      ChordPieces chord;
      chord.init(j, nonstrict_parameter);
      const double lh = static_cast<double>(exact);
      // Declared valid enclosing arithmetic input; not the production affine bound.
      const double error = force_exact || mode != 0 ? std::numeric_limits<double>::infinity()
          : (std::fabs(lh) + 1.0) * 0x1p-40;
      std::cout << '{';
      chord_output(&chord, lh, error, b, exact);
      std::cout << "}\n";
    } else if (command == 'A') {
      int mode = 0;
      std::vector<P3> points(4);
      if (!(std::cin >> mode) || !rounding(mode)) return 2;
      for (auto& p : points) if (!point(&p)) return 2;
      const auto ix = build_cloud_index(points);
      if (!ix.valid) return 2;
      generate_detail::AnchorScratch scratch;
      for (i32 u = 0; u < ix.unique_count(); ++u) scratch.cover.push_back({u, 0});
      const P3& a = points[0];
      const P3& b = points[1];
      const P3& x = points[2];
      const i64 d2 = p3_norm2(p3_sub(b, a));
      const auto form = q3_form(a, b, x);
      if (form.g <= 0) return 2;
      scratch.fill_affine_sites(ix, a, b, d2);
      generate_detail::AffineSeed seed(form, a, b, scratch, float_filter_runtime_enabled());
      size_t target = 0;
      while (target < scratch.cover.size() && ix.upos[static_cast<size_t>(scratch.cover[target].u)] != points[3]) ++target;
      if (target == scratch.cover.size()) return 2;
      const i128 exact = seed.l_exact(scratch, target);
      const i128 power = q3_power(form, points[3]);
      const P3 normal = p3_cross(p3_sub(b, a), p3_sub(x, a));
      const i64 bz = p3_dot(normal, p3_sub(points[3], a));
      const i64 e = p3_norm2(p3_sub(x, a)), xx = p3_norm2(p3_sub(x, b));
      const i128 j = static_cast<i128>(d2) * (3 * form.g - 2 * static_cast<i128>(e) * xx);
      if (j <= 0) return 2;
      ChordPieces chord;
      chord.init(j, nonstrict_parameter);
      std::cout << "{\"L\":" << decimal(exact) << ",\"P\":" << decimal(power)
                << ",\"G\":" << decimal(form.g) << ",\"J\":" << decimal(j)
                << ",\"B\":" << bz << ",\"filter_on\":"
                << (float_filter_runtime_enabled() ? "true" : "false") << ',';
      chord_output(&chord, seed.l_hat(scratch, target), seed.bound, bz, exact);
      std::cout << "}\n";
    } else if (command == 'T') {
      std::vector<P3> points(3);
      for (auto& p : points) if (!point(&p)) return 2;
      const auto ix = build_cloud_index(points);
      if (!ix.valid || ix.has_duplicate_positions()) return 2;
      i32 ua = -1, ub = -1;
      std::vector<CoverPoint> cover;
      for (i32 u = 0; u < ix.unique_count(); ++u) {
        if (ix.point_id(u) == 0) ua = u;
        if (ix.point_id(u) == 1) ub = u;
        cover.push_back({u, 0});
      }
      u64 minimum = 0;
      u32 counts[8]{};
      const bool dead = anchor_sector_kill(cover, ix.upos, ua, ub, points[0], points[1],
          p3_norm2(p3_sub(points[1], points[0])), 8, 1, &minimum, counts, false);
      std::cout << "{\"dead\":" << (dead ? "true" : "false") << ",\"minimum\":" << minimum << ",\"counts\":[";
      for (int i = 0; i < 8; ++i) { if (i) std::cout << ','; std::cout << counts[i]; }
      std::cout << "]}\n";
    } else return 2;
    if (std::fesetround(FE_TONEAREST) != 0) return 2;
  }
  return std::cin.eof() ? 0 : 2;
}
