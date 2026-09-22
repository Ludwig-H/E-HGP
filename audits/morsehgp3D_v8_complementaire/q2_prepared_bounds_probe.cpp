// Independent bounds gate: tests the new header against the exact legacy
// source excerpt and an exhaustive quarter-grid H oracle on small intervals.
#include "spindle/q2_prepared_bounds.hpp"
#include "legacy_bounds.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace {
using mhgp8::Box3;
using mhgp8::Point3;
using mhgp8::Q2Bounds;
using mhgp8::Q2PreparedBounds;
using mhgp8::i64;
using mhgp8::u64;
struct Work {
  u64 preparations{};
  u64 queries{};
  u64 rational_cases{};
  u64 rational_samples{};
  u64 extreme_cases{};
  u64 point_bound_cases{};
  u64 equal_decisions{};
  u64 checked_rejections{};
  u64 cache_split_cases{};
} work;
void require(bool good, const char* message) {
  if (!good) throw std::runtime_error(message);
}
int decision(Q2Bounds b) {
  return b.minimum4 > 0 ? 1 : b.maximum4 <= 0 ? -1 : 0;
}
void same(Q2Bounds a, Q2Bounds b) {
  require(a.minimum4 == b.minimum4 && a.maximum4 == b.maximum4,
          "prepared and legacy extrema differ");
  require(decision(a) == decision(b), "bound decisions differ");
  ++work.equal_decisions;
}
legacy::Q2BallKey pair_key(const Point3& a, const Point3& b) {
  legacy::Q2BallKey key{};
  for (unsigned d = 0; d < 3; ++d) {
    key.center_twice[d] = static_cast<std::uint32_t>(a[d]) + b[d];
    const i64 delta = static_cast<i64>(a[d]) - b[d];
    key.diameter_squared += static_cast<u64>(delta * delta);
  }
  return key;
}
Q2Bounds evaluate(const Q2PreparedBounds& prepared, const Point3& a,
                  const Box3& b, const Box3& z) {
  const auto actual = prepared.bounds(z);
  same(actual, legacy::shared_bounds(a, b, z));
  const auto copied = prepared;
  same(actual, copied.bounds(z));
  if (b.low == b.high) {
    same(actual, legacy::pair_bounds(pair_key(a, b.low), z));
    ++work.point_bound_cases;
  }
  ++work.queries;
  return actual;
}
Point3 axial(unsigned d, std::uint16_t value) {
  std::array<std::uint16_t, 3> p{};
  p[d] = value;
  return {p[0], p[1], p[2]};
}
void small_grid() {
  for (unsigned d = 0; d < 3; ++d) {
    for (unsigned a = 0; a <= 4; ++a) {
      for (unsigned bl = 0; bl <= 4; ++bl) {
        for (unsigned bh = bl; bh <= 4; ++bh) {
          const auto anchor = axial(d, static_cast<std::uint16_t>(a));
          const Box3 b{axial(d, static_cast<std::uint16_t>(bl)),
                       axial(d, static_cast<std::uint16_t>(bh))};
          const Q2PreparedBounds prepared(anchor, b);
          ++work.preparations;
          for (unsigned zl = 0; zl <= 4; ++zl) {
            for (unsigned zh = zl; zh <= 4; ++zh) {
              const Box3 z{axial(d, static_cast<std::uint16_t>(zl)),
                           axial(d, static_cast<std::uint16_t>(zh))};
              const auto actual = evaluate(prepared, anchor, b, z);
              i64 minimum16 = INT64_MAX;
              i64 maximum16 = INT64_MIN;
              for (i64 bv = bl; bv <= bh; ++bv) {
                for (i64 zv4 = 4 * zl; zv4 <= 4 * zh; ++zv4) {
                  const i64 h16 = (zv4 - 4 * a) * (4 * bv - zv4);
                  minimum16 = std::min(minimum16, h16);
                  maximum16 = std::max(maximum16, h16);
                  ++work.rational_samples;
                }
              }
              require(4 * actual.minimum4 == minimum16 &&
                          4 * actual.maximum4 == maximum16,
                      "prepared extrema differ from rational H oracle");
              ++work.rational_cases;
            }
          }
        }
      }
    }
  }
}
void extremes() {
  constexpr std::array<std::uint16_t, 6> values{0, 1, 32767, 32768, 65534, 65535};
  for (unsigned d = 0; d < 3; ++d) {
    for (const auto a : values) {
      for (std::size_t l = 0; l < values.size(); ++l) {
        for (std::size_t h = l; h < values.size(); ++h) {
          const auto anchor = axial(d, a);
          const Box3 b{axial(d, values[l]), axial(d, values[h])};
          const Q2PreparedBounds prepared(anchor, b);
          ++work.preparations;
          for (std::size_t zl = 0; zl < values.size(); ++zl) {
            for (std::size_t zh = zl; zh < values.size(); ++zh) {
              static_cast<void>(evaluate(prepared, anchor, b,
                                         {axial(d, values[zl]), axial(d, values[zh])}));
              ++work.extreme_cases;
            }
          }
        }
      }
    }
  }
  const Point3 a{65535, 65535, 65535};
  const Box3 b{{0, 0, 0}, {65535, 65535, 65535}};
  const Q2PreparedBounds prepared(a, b);
  ++work.preparations;
  same(evaluate(prepared, a, b, b), {-12 * INT64_C(65535) * 65535,
                                    3 * INT64_C(65535) * 65535});
  ++work.extreme_cases;
}
void boxes_3d() {
  u64 seed = UINT64_C(0xb00da31f0579926d);
  const auto next = [&]() {
    seed ^= seed << 13; seed ^= seed >> 7; seed ^= seed << 17;
    return static_cast<std::uint16_t>(seed);
  };
  const auto point = [&]() { const auto x = next(); const auto y = next(); const auto z = next(); return Point3{x, y, z}; };
  const auto box = [&]() {
    const auto p = point(); const auto q = point();
    return Box3{{std::min(p.x, q.x), std::min(p.y, q.y), std::min(p.z, q.z)},
                {std::max(p.x, q.x), std::max(p.y, q.y), std::max(p.z, q.z)}};
  };
  for (unsigned group = 0; group < 512; ++group) {
    const auto a = point(); const auto b = box();
    const Q2PreparedBounds prepared(a, b);
    ++work.preparations;
    for (unsigned i = 0; i < 9; ++i) {
      static_cast<void>(evaluate(prepared, a, b, box()));
    }
  }
}
void caching_and_rejections() {
  const Point3 a{1000, 0, 0};
  const Box3 parent{{60000, 0, 0}, {60004, 0, 0}};
  const Box3 child{{60002, 0, 0}, {60004, 0, 0}};
  const Box3 z{{60001, 0, 0}, {60001, 0, 0}};
  const Q2PreparedBounds p(a, parent), c(a, child);
  work.preparations += 2;
  const auto whole = evaluate(p, a, parent, z);
  const auto narrowed = evaluate(c, a, child, z);
  require(whole.minimum4 == -236004 && whole.maximum4 == 708012 &&
              narrowed.minimum4 == 236004 && narrowed.maximum4 == 708012,
          "cache split fixture changed");
  require(decision(whole) == 0 && decision(narrowed) == 1,
          "reusing parent constants is not an equal-decision child preparation");
  ++work.cache_split_cases;
  const Box3 inverted{{3, 0, 0}, {2, 0, 0}};
  try { static_cast<void>(Q2PreparedBounds(a, inverted)); }
  catch (const std::invalid_argument&) { ++work.checked_rejections; }
  try { static_cast<void>(p.bounds(inverted)); }
  catch (const std::invalid_argument&) { ++work.checked_rejections; }
  require(work.checked_rejections == 2, "invalid box was not rejected");
}
void run() {
  small_grid(); extremes(); boxes_3d(); caching_and_rejections();
  require(work.rational_cases == 3375 && work.rational_samples == 49875 &&
              work.extreme_cases == 7939 && work.queries == 15924 &&
              work.preparations == 1118 && work.point_bound_cases > 3000,
          "prepared bounds gate nonvacuity");
  // Source-level products of nonconstant operands, excluding scaling by 2/4,
  // loop indices and the independent judge. Not compiler instructions/time.
  const u64 legacy_products = 24 * work.queries;
  const u64 prepared_products = 6 * work.preparations + 12 * work.queries;
  std::cout << "{\"status\":\"passed\",\"scope\":\"prepared_bounds_functions_only\""
            << ",\"preparations\":" << work.preparations
            << ",\"queries\":" << work.queries
            << ",\"rational_cases\":" << work.rational_cases
            << ",\"rational_samples\":" << work.rational_samples
            << ",\"extreme_cases\":" << work.extreme_cases
            << ",\"point_bound_cases\":" << work.point_bound_cases
            << ",\"equal_decisions\":" << work.equal_decisions
            << ",\"checked_rejections\":" << work.checked_rejections
            << ",\"cache_split_cases\":" << work.cache_split_cases
            << ",\"prepared_bytes\":" << sizeof(Q2PreparedBounds)
            << ",\"source_level_legacy_products\":" << legacy_products
            << ",\"source_level_prepared_products\":" << prepared_products << "}\n";
}
}  // namespace
int main(int argc, char** argv) {
  if (argc != 2 || std::string_view(argv[1]) != "--selftest") return 2;
  try { run(); return 0; }
  catch (const std::exception& error) { std::cerr << "prepared bounds audit: " << error.what() << '\n'; return 1; }
}
