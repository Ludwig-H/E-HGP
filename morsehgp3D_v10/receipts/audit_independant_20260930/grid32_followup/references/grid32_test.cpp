// Gate autonome, sans assert : identite de grille, pas moteur FULL/u32.
#include <algorithm>
#include <array>
#include <cstdio>
#include <tuple>
#include <vector>

#include "arith/wide.hpp"
#include "cloud/grid32_primitives.hpp"

using namespace mhgp10;
using grid32::PointGrid32;

namespace {
u64 checks = 0, failures = 0, distance_cases = 0, roundtrips = 0;
u64 rejected_keys = 0, duplicate_pairs = 0;

void check(bool condition, const char* message) {
  ++checks;
  if (condition) return;
  ++failures;
  if (failures <= 20) std::printf("ECHEC %s\n", message);
}

// Juge de distance : differences de magnitudes NON signees, produit a
// deux mots via Wide<1>*Wide<1>, puis additions Wide<2> avec retenues.
arith::Wide<2> reference_distance(PointGrid32 a, PointGrid32 b) {
  const u32 av[3] = {a.x, a.y, a.z}, bv[3] = {b.x, b.y, b.z};
  arith::Wide<2> sum;
  for (unsigned axis = 0; axis < 3; ++axis) {
    arith::Wide<1> difference;
    difference.w[0] = av[axis] >= bv[axis] ? u64{av[axis]} - bv[axis]
                                          : u64{bv[axis]} - av[axis];
    const auto square = arith::mul(difference, difference);
    arith::Wide<2> next;
    check(arith::add(sum, square, next), "oracle Wide<2> overflow");
    sum = next;
  }
  return sum;
}

// Juge Morton independant : flux MSB -> LSB, z puis y puis x, et non
// dispersion LSB -> MSB du code produit. Les deux ordres de parcours
// doivent decrire le meme entrelacement ; ordre Morton != ordre lexXYZ.
u128 reference_key(PointGrid32 point) {
  u128 key = 0;
  for (int bit = 31; bit >= 0; --bit) {
    for (u32 coordinate : {point.z, point.y, point.x}) {
      key <<= 1;
      key |= (coordinate >> bit) & 1u;
    }
  }
  return key;
}

void verify_point(PointGrid32 point) {
  ++roundtrips;
  const u128 key = grid32::morton96(point);
  const auto decoded = grid32::decode_morton96(key);
  check(decoded.has_value(), "valid key refused");
  check(decoded.has_value() && *decoded == point, "point roundtrip");
  check((key >> 96) == 0, "encoder high bits");
  check(key == reference_key(point), "independent Morton stream");
  if (decoded) check(grid32::morton96(*decoded) == key, "key roundtrip");
}

void verify_distance(PointGrid32 a, PointGrid32 b) {
  ++distance_cases;
  const u128 distance = grid32::squared_distance(a, b);
  const auto expected = reference_distance(a, b);
  check(arith::cmp(expected, arith::Wide<2>::from_u128(distance)) == 0,
        "independent distance Wide<2>");
  check(distance == grid32::squared_distance(b, a), "distance symmetry");
  check(grid32::squared_distance(a, a) == 0, "distance identity");
  check((distance == 0) == (a == b), "distance separates positions");
  check(distance <= grid32::kMaxSquaredDistance, "distance bound");
}

PointGrid32 permute(PointGrid32 point, const std::array<unsigned, 3>& order) {
  const u32 xyz[3] = {point.x, point.y, point.z};
  return {xyz[order[0]], xyz[order[1]], xyz[order[2]]};
}

u32 random_word(u64& state) {
  // SplitMix64, arithmetique non signee modulo 2^64, graine fixe.
  state += 0x9e3779b97f4a7c15ull;
  u64 word = state;
  word = (word ^ (word >> 30)) * 0xbf58476d1ce4e5b9ull;
  word = (word ^ (word >> 27)) * 0x94d049bb133111ebull;
  return static_cast<u32>(word ^ (word >> 31));
}

bool lex_less(PointGrid32 a, PointGrid32 b) {
  return std::tie(a.x, a.y, a.z) < std::tie(b.x, b.y, b.z);
}
}  // namespace

int main() {
  constexpr u32 maximum = 0xffffffffu;
  const PointGrid32 origin{0, 0, 0}, cube{maximum, maximum, maximum};
  verify_point(origin);
  verify_point(cube);
  verify_distance(origin, cube);
  check(arith::to_string(arith::Wide<2>::from_u128(grid32::squared_distance(origin, cube))) ==
            "55340232195358851075",
        "independent decimal cube maximum");
  check(grid32::squared_distance(origin, cube) == grid32::kMaxSquaredDistance,
        "maximum-distance constant");
  check(grid32::kMaxSquaredDistance > u128{~u64{0}}, "u64 cannot hold cube maximum");
  check(grid32::morton96(cube) == grid32::kMaxMortonKey, "96 ones key");

  const PointGrid32 formerly_colliding{u32{1} << 21, 0, 0};
  check(grid32::morton96(origin) != grid32::morton96(formerly_colliding),
        "old 21-bit collision eliminated");
  verify_point(formerly_colliding);
  verify_distance(origin, formerly_colliding);

  for (unsigned bit = 0; bit < 32; ++bit) {
    for (unsigned axis = 0; axis < 3; ++axis) {
      u32 xyz[3] = {0, 0, 0};
      xyz[axis] = u32{1} << bit;
      const PointGrid32 point{xyz[0], xyz[1], xyz[2]};
      check(grid32::morton96(point) == (u128{1} << (3 * bit + axis)), "axis bit position");
      verify_point(point);
      verify_distance(origin, point);
      xyz[axis] = (u32{1} << bit) - 1;
      verify_point({xyz[0], xyz[1], xyz[2]});
    }
  }
  std::array<PointGrid32, 8> corners;
  for (unsigned i = 0; i < 8; ++i) {
    corners[i] = {(i & 1) ? maximum : 0u, (i & 2) ? maximum : 0u, (i & 4) ? maximum : 0u};
    verify_point(corners[i]);
  }
  for (PointGrid32 a : corners)
    for (PointGrid32 b : corners) verify_distance(a, b);

  for (unsigned bit = 96; bit < 128; ++bit) {
    for (u128 low : {u128{0}, grid32::kMaxMortonKey}) {
      ++rejected_keys;
      check(!grid32::decode_morton96((u128{1} << bit) | low), "out-of-domain key accepted");
    }
  }

  std::vector<PointGrid32> points;
  points.reserve(10000);
  u64 state = 0x09302026abcdef01ull;
  for (unsigned i = 0; i < 10000; ++i) {
    if (i != 0 && i % 97 == 0) points.push_back(points[i / 2]);
    else points.push_back({random_word(state), random_word(state), random_word(state)});
  }
  const std::array<std::array<unsigned, 3>, 6> permutations = {{
      {{0, 1, 2}}, {{0, 2, 1}}, {{1, 0, 2}}, {{1, 2, 0}}, {{2, 0, 1}}, {{2, 1, 0}}}};
  for (unsigned i = 0; i < points.size(); ++i) {
    const PointGrid32 a = points[i], b = points[(i + 1) % points.size()];
    verify_point(a);
    verify_distance(a, b);
    const u128 expected = grid32::squared_distance(a, b);
    for (const auto& order : permutations)
      check(grid32::squared_distance(permute(a, order), permute(b, order)) == expected,
            "axis permutation invariance");
    const PointGrid32 small_a{a.x & 0x3fffffffu, a.y & 0x3fffffffu, a.z & 0x3fffffffu};
    const PointGrid32 small_b{b.x & 0x3fffffffu, b.y & 0x3fffffffu, b.z & 0x3fffffffu};
    const PointGrid32 shift{0xc0000000u, 0xa0000000u, 0x80000000u};
    const u64 ax = u64{small_a.x} + shift.x, ay = u64{small_a.y} + shift.y,
              az = u64{small_a.z} + shift.z;
    const u64 bx = u64{small_b.x} + shift.x, by = u64{small_b.y} + shift.y,
              bz = u64{small_b.z} + shift.z;
    check(ax <= maximum && ay <= maximum && az <= maximum &&
              bx <= maximum && by <= maximum && bz <= maximum,
          "translation stays in u32 domain");
    check(grid32::squared_distance({u32(ax), u32(ay), u32(az)}, {u32(bx), u32(by), u32(bz)}) ==
              grid32::squared_distance(small_a, small_b),
          "common translation invariance");
  }

  // Identite jugee par tri lexXYZ INDEPENDANT ; aucun ordre Morton=lexXYZ promis.
  std::vector<PointGrid32> lex = points;
  std::sort(lex.begin(), lex.end(), lex_less);
  u64 unique_xyz = 1;
  for (std::size_t i = 1; i < lex.size(); ++i) {
    if (lex[i] == lex[i - 1]) {
      ++duplicate_pairs;
      check(grid32::morton96(lex[i]) == grid32::morton96(lex[i - 1]), "same XYZ same key");
    } else ++unique_xyz;
  }
  std::vector<PointGrid32> keyed = points;
  const auto key_less = [](PointGrid32 a, PointGrid32 b) { return grid32::morton96(a) < grid32::morton96(b); };
  std::sort(keyed.begin(), keyed.end(), key_less);
  u64 unique_keys = 1;
  for (std::size_t i = 1; i < keyed.size(); ++i) {
    if (grid32::morton96(keyed[i]) == grid32::morton96(keyed[i - 1]))
      check(keyed[i] == keyed[i - 1], "different XYZ key collision");
    else ++unique_keys;
  }
  check(unique_keys == unique_xyz, "key identity vs lexXYZ identity");
  std::reverse(points.begin(), points.end());
  std::rotate(points.begin(), points.begin() + 317, points.end());
  std::sort(points.begin(), points.end(), key_less);
  check(points == keyed, "input permutation canonical keys");

  std::printf("grille32_checks %llu distances %llu roundtrips %llu refused_keys %llu duplicates %llu\n",
              static_cast<unsigned long long>(checks), static_cast<unsigned long long>(distance_cases),
              static_cast<unsigned long long>(roundtrips), static_cast<unsigned long long>(rejected_keys),
              static_cast<unsigned long long>(duplicate_pairs));
  if (checks < 200000 || distance_cases < 10000 || roundtrips < 10000 ||
      rejected_keys != 64 || duplicate_pairs < 100) return 3;
  if (failures != 0) return 1;
  std::puts("grille32_ok");
  return 0;
}
