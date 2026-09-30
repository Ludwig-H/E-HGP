// Observational micro-probe of unchanged u18 bodies OUTSIDE their contract.
// This is not an engine build, and success means only that the call returned.
#include <array>
#include <iostream>
#include <string>

#include "arith/geometry.hpp"
#include "snippets/center_in_box.hpp"

using namespace mhgp10;
using geom::P3;

namespace {
std::string integer(i128 value) {
  return arith::to_string(arith::I128w::from_i128(value));
}
void point(P3 value) {
  std::cout << '[' << value.x << ',' << value.y << ',' << value.z << ']';
}
void center(const geom::Center& value) {
  std::cout << "{\"N\":[\"" << integer(value.N[0]) << "\",\"" << integer(value.N[1])
            << "\",\"" << integer(value.N[2]) << "\"],\"D\":\"" << integer(value.D) << "\"}";
}
}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) return 2;
  const std::string mode = argv[1];
  const int bits = std::stoi(argv[2]);
  if (bits < 1 || bits > 32) return 2;
  const i64 m = static_cast<i64>((u64{1} << bits) - 1);
  const P3 a{0, 0, 0}, b{m, m, 0}, c{m, 0, m}, d{0, m, m}, cube{m, m, m};
  std::array<P3, 4> support{a, b, c, d};
  unsigned arity = 4;
  geom::Center ctr{};
  geom::Level level;
  std::array<P3, 3> queries{};
  std::array<int, 3> sides{};
  std::array<i128, 3> keys{};
  unsigned query_count = 0;
  bool constructed = true, ownership = false, have_level = false;
  const micro_owner::Box box{{0, 0, 0}, {64 * m + 1, 64 * m + 1, 64 * m + 1}};
  std::array<int, 3> orientations{};
  if (mode == "orient") {
    orientations = {geom::orient(a, b, c, d), geom::orient(a, c, b, d), geom::orient(a, b, c, a)};
  } else if (mode == "side2" || mode == "level2") {
    arity = 2;
    support[1] = cube;
    geom::center2(a, cube, ctr);
    if (mode == "level2") {
      level = geom::level2(a, cube);
      have_level = true;
    } else {
      queries = {a, cube, P3{m / 2, m / 2, m / 2}};
      query_count = 3;
    }
  } else if (mode == "side3" || mode == "box3") {
    arity = 3;
    constructed = geom::center3(a, b, c, ctr);
    if (mode == "side3") {
      queries[0] = d;
      query_count = 1;
    } else ownership = micro_owner::center_in_box(a, ctr, box);
  } else if (mode == "level4" || mode == "center4") {
    constructed = geom::center4(a, b, c, d, ctr);
    if (mode == "level4") {
      level = geom::level4(ctr);
      have_level = true;
    }
  } else return 2;
  for (unsigned i = 0; i < query_count; ++i) {
    sides[i] = geom::side(ctr, a, queries[i]);
    keys[i] = geom::side_key(ctr, a, queries[i]);
  }
  std::cout << "{\"mode\":\"" << mode << "\",\"bits\":" << bits
            << ",\"native_status\":\"observational\",\"support\":[";
  for (unsigned i = 0; i < arity; ++i) {
    if (i) std::cout << ',';
    point(support[i]);
  }
  std::cout << ']';
  if (mode == "orient") {
    std::cout << ",\"orientations\":[" << orientations[0] << ',' << orientations[1]
              << ',' << orientations[2] << ']';
  } else {
    std::cout << ",\"constructed\":" << (constructed ? "true" : "false") << ",\"center\":";
    center(ctr);
    if (have_level)
      std::cout << ",\"level\":{\"num\":\"" << arith::to_string(level.num)
                << "\",\"den\":\"" << arith::to_string(level.den) << "\"}";
    if (mode == "box3") {
      std::cout << ",\"box\":{\"lo\":[0,0,0],\"hi\":[" << box.hi[0] << ',' << box.hi[1]
                << ',' << box.hi[2] << "]},\"ownership\":" << (ownership ? "true" : "false");
    }
    if (query_count) {
      std::cout << ",\"queries\":[";
      for (unsigned i = 0; i < query_count; ++i) {
        if (i) std::cout << ',';
        std::cout << "{\"xyz\":";
        point(queries[i]);
        std::cout << ",\"side\":" << sides[i] << ",\"key\":\"" << integer(keys[i]) << "\"}";
      }
      std::cout << ']';
    }
  }
  std::cout << "}\n";
  return 0;
}
