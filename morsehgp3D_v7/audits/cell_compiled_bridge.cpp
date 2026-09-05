// Audit-only transport: real cell helpers, judged independently in Python.
#include <cfenv>
#include <iostream>
#include <limits>
#include <string>

#include "../src/pipeline/generate.hpp"

using namespace mhgp7;

static void integer(i128 v) {
  if (v < 0) { std::cout << '-'; v = -v; }
  std::string digits;
  do { digits += static_cast<char>('0' + v % 10); v /= 10; } while (v);
  for (auto it = digits.rbegin(); it != digits.rend(); ++it) std::cout << *it;
}

static void box(bool ok, const int r[4]) {
  std::cout << '[' << ok;
  for (int k = 0; k < 4; ++k) std::cout << ',' << (ok ? r[k] : 0);
  std::cout << ']';
}

template <int G>
static void grid_case(int rho, u64 h, const std::vector<P3>& points) {
  CellGridT<G> grid;
  const P3 a = points[0], b = points[1], d = p3_sub(b, a);
  const i64 dvec[3] = {d.x, d.y, d.z};
  const i64 D2 = p3_norm2(d);
  std::vector<CoverPoint> cover;
  for (size_t k = 0; k < points.size(); ++k) cover.push_back({static_cast<i32>(k), 0});
  if (!grid.build(cover, points, 0, 1, a, b, D2, rho, h, float_filter_runtime_enabled()))
    throw std::runtime_error("nominal grid refused");
  std::cout << "{\"u\":[";
  for (int k = 0; k < 3; ++k) std::cout << (k ? "," : "") << grid.u[k];
  std::cout << "],\"v\":[";
  for (int k = 0; k < 3; ++k) std::cout << (k ? "," : "") << grid.v[k];
  std::cout << "],\"metadata\":[" << grid.h << ',' << grid.needed_cells << ',' << grid.dead_cells << ',' << grid.all_dead;
  std::cout << "],\"counts\":[";
  for (int j = 0; j < grid.NC; ++j)
    for (int i = 0; i < grid.NC; ++i) std::cout << ((j || i) ? "," : "") << grid.cnt[j][i];
  int r[4] = {};
  std::cout << "],\"origin\":";
  const bool origin_ok = grid.locate_box(0, 0, 1, r);
  box(origin_ok, r);
  std::cout << ",\"seeds\":[";
  bool first = true;
  for (size_t k = 2; k < points.size(); ++k) {
    const P3 x = points[k];
    if (!is_acute_seed(a, b, x, D2, 0, 1, k)) continue;
    const auto f = q3_form(a, b, x);
    i128 pu = 0, pv = 0, den = 0;
    generate_detail::seed_center_coords(grid, f, dvec, &pu, &pv, &den);
    std::cout << (first ? "" : ",") << "{\"index\":" << k << ",\"center\":[";
    first = false;
    integer(pu); std::cout << ','; integer(pv); std::cout << ','; integer(den);
    std::cout << "],\"center_box\":";
    const bool center_ok = grid.locate_box(pu, pv, den, r);
    box(center_ok, r);
    i128 p0 = 0, q0 = 0, p1 = 0, q1 = 0;
    const bool chord = generate_detail::seed_chord_coords(
        grid, f, dvec, p3_cross(d, p3_sub(x, a)), D2, p3_norm2(p3_sub(x, a)),
        p3_norm2(p3_sub(x, b)), &p0, &q0, &p1, &q1, &den);
    std::cout << ",\"chord\":[" << chord << ',';
    integer(p0); std::cout << ','; integer(q0); std::cout << ',';
    integer(p1); std::cout << ','; integer(q1); std::cout << ','; integer(den);
    std::cout << "],\"chord_box\":";
    const bool chord_ok = chord && grid.segment_box(p0, q0, p1, q1, den, r);
    box(chord_ok, r);
    std::cout << '}';
  }
  std::cout << "],\"environment_rejects\":[";
  int ordinal = 0;
  for (int mode : {FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO}) {
    if (std::fesetround(mode)) throw std::runtime_error("fesetround");
    const bool ok = grid.build(cover, points, 0, 1, a, b, D2, rho, h, float_filter_runtime_enabled());
    std::cout << (ordinal++ ? "," : "") << (!ok && !grid.built && grid.fail == CellGridT<G>::Fail::kEnvironment);
  }
  if (std::fesetround(FE_TONEAREST)) throw std::runtime_error("restore round");
  std::cout << "],\"domain_rejects\":[";
  const double inf = std::numeric_limits<double>::infinity();
  const double nan = std::numeric_limits<double>::quiet_NaN();
  std::cout << !grid.range_in_domain(nan, 0) << ',' << !grid.range_in_domain(0, nan) << ','
            << !grid.range_in_domain(-inf, 0) << ',' << !grid.range_in_domain(0, inf) << ','
            << !grid.range_in_domain(1, 0) << ',' << !grid.range_in_domain(-4.0 * G - 1, 0) << "]}\n";
}

template <int G, typename Int>
static void synthetic(i128 du, i128 dv, i128 rhs) {
  using Grid = CellGridT<G>;
  u32 dlo[Grid::NC][Grid::NC + 1] = {}, dhi[Grid::NC][Grid::NC + 1] = {}, out[Grid::NC][Grid::NC] = {};
  Grid::template count_site<Int>(static_cast<Int>(du), static_cast<Int>(dv), static_cast<Int>(rhs), false, dlo, dhi);
  Grid::accumulate(dlo, dhi, out);
  std::cout << '[';
  for (int j = 0; j < Grid::NC; ++j)
    for (int i = 0; i < Grid::NC; ++i) std::cout << ((j || i) ? "," : "") << out[j][i];
  std::cout << "]\n";
}

static i128 read_integer() {
  std::string s;
  if (!(std::cin >> s)) throw std::runtime_error("missing integer");
  i128 n = 0;
  for (char c : s) {
    if (c == '-' && n == 0) continue;
    if (c < '0' || c > '9') throw std::runtime_error("invalid integer");
    n = 10 * n + (c - '0');
  }
  return s[0] == '-' ? -n : n;
}

int main(int argc, char** argv) {
  try {
    if (argc == 2 && !mutants_enable(argv[1])) return 2;
    if (argc > 2 || std::fesetround(FE_TONEAREST)) return 2;
    char command;
    while (std::cin >> command) {
      int G;
      std::cin >> G;
      if (G != 8 && G != 16) return 2;
      if (command == 'C') {
        int rho;
        u64 h;
        size_t n;
        std::cin >> rho >> h >> n;
        if (n < 2 || n > 128 || (rho != 8 && rho != 12)) return 2;
        std::vector<P3> points(n);
        for (auto& p : points) {
          std::cin >> p.x >> p.y >> p.z;
          if (p.x < 0 || p.y < 0 || p.z < 0 || p.x > 65535 || p.y > 65535 || p.z > 65535) return 2;
        }
        if (G == 8) grid_case<8>(rho, h, points); else grid_case<16>(rho, h, points);
      } else if (command == 'S') {
        int width;
        std::cin >> width;
        const i128 du = read_integer(), dv = read_integer(), rhs = read_integer();
        if (width == 64) {
          if (G == 8) synthetic<8, i64>(du, dv, rhs); else synthetic<16, i64>(du, dv, rhs);
        } else if (width == 128) {
          if (G == 8) synthetic<8, i128>(du, dv, rhs); else synthetic<16, i128>(du, dv, rhs);
        } else return 2;
      } else return 2;
      if (!std::cin) return 2;
    }
    return 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 2;
  }
}
