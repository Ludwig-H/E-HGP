#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string_view>

#include "pipeline/local_credits.hpp"

namespace mhgp8::bench {

// Recipe v1 extracted explicitly from p0_probe.cpp. Coordinates, original
// IDs and FNV byte order are unchanged; old receipts are not requalified.
namespace fixture_detail {

inline std::size_t grid_side(std::size_t count, unsigned dimension) {
  std::size_t side = 1;
  while ((dimension == 2 ? side * side : side * side * side) < count) {
    ++side;
  }
  return side;
}

inline void append_factor(RectangleInput& input, std::size_t count,
                          Coordinate base_x, std::string_view family) {
  const auto side = family == "tube" ? std::size_t{1} :
      grid_side(count, family == "sheet" ? 2U : 3U);
  for (std::size_t index = 0; index < count; ++index) {
    std::size_t x = base_x;
    std::size_t y = 1000;
    std::size_t z = 1000;
    if (family == "tube") {
      x += index;
    } else if (family == "sheet") {
      y += index % side;
      z += index / side;
    } else {
      x += index % side;
      y += (index / side) % side;
      z += index / (side * side);
    }
    if (std::max({x, y, z}) > static_cast<std::size_t>(coordinate_limit)) {
      throw std::logic_error("fixture coordinate escaped its declared coordinate domain");
    }
    input.points.push_back({static_cast<Coordinate>(x),
                            static_cast<Coordinate>(y),
                            static_cast<Coordinate>(z)});
  }
}

}  // namespace fixture_detail

[[nodiscard]] inline RectangleInput make_fixture(std::size_t n,
                                                  std::string_view family) {
  if (n < 2 || (family != "grid" && family != "sheet" && family != "skew" &&
                family != "tube" && family != "rails")) {
    throw std::invalid_argument("require n>=2 and a declared P0 fixture family");
  }
  if (family == "rails") {
    if (n != 2718) {
      throw std::invalid_argument("rails version 1 is the fixed n=2718 auditor counter-fixture");
    }
    RectangleInput result;
    for (unsigned side = 0; side < 2; ++side) {
      for (unsigned rail = 0; rail < 9; ++rail) {
        for (unsigned x = 0; x <= 150; ++x) {
          result.points.push_back({static_cast<Coordinate>(64800 * side + x),
                                   static_cast<Coordinate>(600 * rail), 0});
        }
      }
    }
    result.a = {0, 1359};
    result.b = {1359, 2718};
    return result;
  }
  const auto b_count = family == "skew"
      ? std::max(std::size_t{1}, n / 16) : n - n / 2;
  const auto a_count = n - b_count;
  // Fixed recipe domains, never a truncation quota imposed on the producer.
  const std::size_t capacity = family == "sheet" ? 256 * 256 :
      family == "tube" ? 65536 - 60000 : 32 * 32 * 32;
  if (std::max(a_count, b_count) > capacity) {
    throw std::invalid_argument(
        "requested n exceeds the declared coordinate capacity of this fixture family");
  }
  RectangleInput input;
  input.points.reserve(n);
  fixture_detail::append_factor(input, a_count, 1000, family);
  fixture_detail::append_factor(input, b_count, 60000, family);
  input.a = {0, a_count};
  input.b = {a_count, n};
  return input;
}

[[nodiscard]] inline std::uint64_t fixture_hash(const RectangleInput& input) {
  // FNV-1a over x,y,z as little-endian u16, in original point order.
  // Unsigned wrap is part of this reproducible checksum, not certification.
  std::uint64_t hash = 14695981039346656037ULL;
  for (const auto& point : input.points) {
    for (std::size_t axis = 0; axis < 3; ++axis) {
      const auto coordinate = point[axis];
      hash ^= coordinate & 255U;
      hash *= 1099511628211ULL;
      hash ^= coordinate >> 8U;
      hash *= 1099511628211ULL;
    }
  }
  return hash;
}

}  // namespace mhgp8::bench
