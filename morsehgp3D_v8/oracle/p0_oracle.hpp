#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

#include <boost/multiprecision/cpp_int.hpp>

#include "core/types.hpp"

// Bounded test oracle only. No production geometry, block classifier or
// credit planner is called here. This judges Wq and box-universal credits,
// not Gabriel census, support completeness or the HGP FULL tower.
namespace mhgp8::oracle {

using Integer = boost::multiprecision::cpp_int;

struct Metrics {
  Integer h;
  Integer xi;
};

[[nodiscard]] inline Metrics metrics(const Point3& a, const Point3& b,
                                     const Point3& z) {
  std::array<Integer, 3> u{};
  std::array<Integer, 3> v{};
  Integer uu = 0;
  Integer vv = 0;
  Integer uv = 0;
  for (std::size_t axis = 0; axis < 3; ++axis) {
    u[axis] = Integer(z[axis]) - Integer(a[axis]);
    v[axis] = Integer(b[axis]) - Integer(a[axis]);
    uu += u[axis] * u[axis];
    vv += v[axis] * v[axis];
    uv += u[axis] * v[axis];
  }
  // Gram determinant, independently of the production cross products.
  return {uv - uu, uu * vv - uv * uv};
}

[[nodiscard]] inline bool point_witness(Lane lane, const Point3& a,
                                       const Point3& b, const Point3& z) {
  const auto q = static_cast<unsigned>(lane);
  if (q < 2 || q > 4) {
    throw std::invalid_argument("oracle: invalid lane");
  }
  const Metrics value = metrics(a, b, z);
  if (value.h <= 0) {
    return false;
  }
  if (q == 2) {
    return true;
  }
  const unsigned coefficient = q == 3 ? 3 : 2;
  return coefficient * value.h * value.h > value.xi;
}

[[nodiscard]] inline std::array<Point3, 8> corners(const Box3& box) {
  std::array<Point3, 8> result{};
  std::size_t offset = 0;
  for (const auto x : {box.low.x, box.high.x}) {
    for (const auto y : {box.low.y, box.high.y}) {
      for (const auto z : {box.low.z, box.high.z}) {
        result[offset++] = {x, y, z};
      }
    }
  }
  return result;
}

[[nodiscard]] inline bool universal_witness(Lane lane, const Point3& a,
                                           const Box3& b, const Point3& z) {
  for (const auto& endpoint : corners(b)) {
    if (!point_witness(lane, a, endpoint, z)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline bool box_witness(Lane lane, const Box3& a,
                                     const Box3& b, const Point3& z) {
  for (const auto& endpoint : corners(a)) {
    if (!universal_witness(lane, endpoint, b, z)) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] inline Box3 bounds(std::span<const Point3> points,
                                std::size_t first, std::size_t last) {
  if (first >= last || last > points.size()) {
    throw std::invalid_argument("oracle: invalid range");
  }
  Box3 result{points[first], points[first]};
  for (std::size_t i = first + 1; i < last; ++i) {
    result.low.x = std::min(result.low.x, points[i].x);
    result.low.y = std::min(result.low.y, points[i].y);
    result.low.z = std::min(result.low.z, points[i].z);
    result.high.x = std::max(result.high.x, points[i].x);
    result.high.y = std::max(result.high.y, points[i].y);
    result.high.z = std::max(result.high.z, points[i].z);
  }
  return result;
}

[[nodiscard]] inline unsigned threshold(unsigned kmax, Lane lane) {
  const auto q = static_cast<unsigned>(lane);
  return q > kmax + 1 ? 0 : kmax + 2 - q;
}

[[nodiscard]] inline unsigned core_credit(
    std::span<const Point3> points, std::span<const std::size_t> proposed,
    const Box3& a, const Box3& b, Lane lane, unsigned cap) {
  unsigned credit = 0;
  for (const auto id : proposed) {
    if (box_witness(lane, a, b, points[id])) {
      ++credit;
    }
  }
  return std::min(credit, cap);
}

[[nodiscard]] inline std::vector<std::uint8_t> local_credits(
    std::span<const Point3> points, std::size_t first, std::size_t last,
    const Box3& opposite, Lane lane, unsigned cap) {
  std::vector<std::uint8_t> result(last - first, 0);
  for (std::size_t a = first; a < last; ++a) {
    unsigned count = 0;
    for (std::size_t z = first; z < last; ++z) {
      if (z != a && universal_witness(lane, points[a], opposite, points[z])) {
        ++count;
      }
    }
    result[a - first] = static_cast<std::uint8_t>(std::min(count, cap));
  }
  return result;
}

[[nodiscard]] inline unsigned point_credit(
    std::span<const Point3> points, std::size_t a, std::size_t b,
    Lane lane, unsigned cap) {
  unsigned count = 0;
  for (std::size_t z = 0; z < points.size() && count < cap; ++z) {
    if (z != a && z != b && point_witness(lane, points[a], points[b], points[z])) {
      ++count;
    }
  }
  return count;
}

}  // namespace mhgp8::oracle
